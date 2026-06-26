#include "pch.h"
#include "Session.h"
#include "ZombieManager.h"
#include "ServerSpatialGrid.h"

extern ZombieManager                g_ZombieManager;
extern ServerSpatialGrid            g_SpatialGrid;
extern concurrency::concurrent_unordered_map<int, Vector3> g_PlayerPositions;

void Session::init(SOCKET s, int id, Room* room)
{
	m_client = s;
	m_id = id;
	m_is_connected = true;
	memset(&m_transform, 0, sizeof(m_transform));
	m_room = room;
	m_prev_recv = 0;
}

void Session::do_recv()
{
	DWORD recv_flag = 0;
	memset(&m_recv_over.m_over, 0, sizeof(m_recv_over.m_over));

	// 패킷 잘릴 수 있어서
	m_recv_over.m_wsa.buf = m_recv_over.m_buff + m_prev_recv;
	m_recv_over.m_wsa.len = BUF_SIZE - m_prev_recv;

	WSARecv(m_client, &m_recv_over.m_wsa, 1, 0, &recv_flag, &m_recv_over.m_over, nullptr);
}

void Session::do_send(int num_bytes, char* mess)
{
	EXP_OVER* o = new EXP_OVER(IO_SEND);
	o->m_wsa.len = num_bytes;
	memcpy(o->m_buff, mess, num_bytes);
	WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
}

void Session::send_add_player(int player_id)
{
	S2C_AddPlayer packet;
	packet.size = sizeof(S2C_AddPlayer);
	packet.type = S2C_ADD_PLAYER;
	packet.playerId = player_id;
	Session& pl = clients[player_id];
	memcpy(packet.username, pl.m_username, sizeof(packet.username));
	packet.transform = pl.m_transform;
	packet.bRunning = pl.m_bRunning;
	packet.bAiming = pl.m_bAiming;
	packet.fAimPitch = pl.m_fAimPitch;
	packet.weaponType = pl.m_weaponType;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_avatar_info()
{
	S2C_AvatarInfo packet;
	packet.size = sizeof(S2C_AvatarInfo);
	packet.type = S2C_AVATAR_INFO;
	packet.playerId = m_id;
	packet.transform = m_transform;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_transform_packet(int mover)
{
	if (m_room == nullptr) return;

	S2C_Transform packet;
	packet.size = sizeof(S2C_Transform);
	packet.type = S2C_TRANSFORM;
	packet.playerId = mover;
	packet.transform = clients[mover].m_transform;
	packet.bRunning = clients[mover].m_bRunning;
	packet.bAiming = clients[mover].m_bAiming;
	packet.fAimPitch = clients[mover].m_fAimPitch;

	for (int p : m_room->players) {
		if (p == -1) continue;
		if (p == mover) continue;
		clients[p].do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
}

void Session::send_login_success()
{
	S2C_LoginResult packet;
	packet.size = sizeof(S2C_LoginResult);
	packet.type = S2C_LOGIN_RESULT;
	packet.success = true;
	strncpy_s(packet.message, "Login successful.", sizeof(packet.message));
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_remove_player(int player_id)
{
	S2C_RemovePlayer packet;
	packet.size = sizeof(S2C_RemovePlayer);
	packet.type = S2C_REMOVE_PLAYER;
	packet.playerId = player_id;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_player_reload(int player_id)
{
	S2C_PlayerReload packet;
	packet.size = sizeof(S2C_PlayerReload);
	packet.type = S2C_PLAYER_RELOAD;
	packet.playerId = player_id;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

bool Session::process_packet(unsigned char* p)
{
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
	switch (type) {
	case C2S_LOGIN: {
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
		strncpy_s(m_username, packet->username, MAX_NAME_LEN);
		std::cout << "Player[" << m_id << "] logged in as " << m_username << std::endl;
		send_avatar_info();

		// 이미 스폰된 좀비 목록을 신규 클라이언트에게 전송
		for (auto& [nZombieId, zombie] : g_ZombieManager.GetZombies())
		{
			if (!zombie.bAlive || !zombie.pAgent) continue;
			send_spawn_zombie(nZombieId, zombie.pAgent->GetPosition());
		}
	}
	break;
	case C2S_TRANSFORM: {
		C2S_Transform* packet = reinterpret_cast<C2S_Transform*>(p);
		m_transform = packet->transform;
		m_bRunning = packet->bRunning;
		m_bAiming = packet->bAiming;
		m_fAimPitch = packet->fAimPitch;

		float x = m_transform.m[3][0];
		float y = m_transform.m[3][1];
		float z = m_transform.m[3][2];

		// 좀비 AI용 플레이어 위치 갱신
		g_PlayerPositions[m_id] = Vector3{ x, y, z };
		//std::cout << "[Transform] Player[" << m_id << "] pos(" << x << "," << y << "," << z << ")\n";

		send_transform_packet(m_id);
	}
					  break;
	case C2S_PLAYER_SHOOT: {
		C2S_PlayerShoot* packet = reinterpret_cast<C2S_PlayerShoot*>(p);

		Vector3 v3Origin{ packet->originX, packet->originY, packet->originZ };
		Vector3 v3Dir{ packet->dirX, packet->dirY, packet->dirZ };
		v3Dir.Normalize();

		//std::cout << "[Shoot] Player[" << m_id << "] origin("
		//          << v3Origin.x << "," << v3Origin.y << "," << v3Origin.z
		//          << ") dir(" << v3Dir.x << "," << v3Dir.y << "," << v3Dir.z << ")\n";

		constexpr float fMaxDist = 5000.f;
		const float     fDamage  = packet->damage; // 무기/펠릿 데미지 (클라 전송)

		// ── 1. 좀비 히트 검사 (BoundingSphere) ──────────────────────────────
		int   nHitZombieId = -1;
		float fZombieDist  = fMaxDist;
		g_ZombieManager.RayTestZombies(v3Origin, v3Dir, fMaxDist, nHitZombieId, fZombieDist);

		// ── 2. 정적 오브젝트 히트 검사 (OBB 그리드) ─────────────────────────
		ServerRayHitResult staticHit;
		g_SpatialGrid.RayTestStatics(v3Origin, v3Dir, fMaxDist, staticHit);

		//std::cout << "[Shoot] zombieHit=" << nHitZombieId
		//          << " zombieDist=" << fZombieDist
		//          << " staticHit=" << staticHit.bHit
		//          << " staticDist=" << staticHit.fDistance << "\n";

		// ── 3. 가장 가까운 히트 채택 ────────────────────────────────────────
		S2C_ShootResult result{};
		result.size = sizeof(S2C_ShootResult);
		result.type = S2C_SHOOT_RESULT;
		result.shooterPlayerId = m_id;
		result.hitZombieId = -1;
		result.damage = 0.f;
		result.bHit = 0;
		result.muzzleX  = packet->muzzleX;
		result.muzzleY  = packet->muzzleY;
		result.muzzleZ  = packet->muzzleZ;
		result.shootDirX = v3Dir.x;
		result.shootDirY = v3Dir.y;
		result.shootDirZ = v3Dir.z;

		bool bZombieCloser = (nHitZombieId >= 0) &&
			(!staticHit.bHit || fZombieDist <= staticHit.fDistance);

		if (bZombieCloser)
		{
			// 좀비와 사격자 사이에 벽이 있는지 추가 차폐 체크
			Vector3 v3ZombieHitPoint = v3Origin + v3Dir * fZombieDist;
			bool bBlocked = g_SpatialGrid.IsLineOfSightBlocked(v3Origin, v3ZombieHitPoint);

			if (!bBlocked)
			{
				result.bHit = 2; // zombie
				result.hitX = v3Origin.x + v3Dir.x * fZombieDist;
				result.hitY = v3Origin.y + v3Dir.y * fZombieDist;
				result.hitZ = v3Origin.z + v3Dir.z * fZombieDist;
				result.hitNormalX = -v3Dir.x;
				result.hitNormalY = -v3Dir.y;
				result.hitNormalZ = -v3Dir.z;
				result.hitZombieId = nHitZombieId;
				result.damage = fDamage;

				// 데미지 적용 (사망 시 bAlive=false로 AI 중단, despawn은 보내지 않음)
				// 클라이언트가 TakeDamage → 사망 몽타주 → 자체 제거 처리
				g_ZombieManager.ApplyDamageToZombie(nHitZombieId, fDamage);
			}
			else if (staticHit.bHit)
			{
				// 벽에 차폐된 경우 → 벽 히트로 처리
				result.bHit = 1; // static
				result.hitX = staticHit.v3HitPoint.x;
				result.hitY = staticHit.v3HitPoint.y;
				result.hitZ = staticHit.v3HitPoint.z;
				result.hitNormalX = staticHit.v3HitNormal.x;
				result.hitNormalY = staticHit.v3HitNormal.y;
				result.hitNormalZ = staticHit.v3HitNormal.z;
			}
		}
		else if (staticHit.bHit)
		{
			result.bHit = 1; // static
			result.hitX = staticHit.v3HitPoint.x;
			result.hitY = staticHit.v3HitPoint.y;
			result.hitZ = staticHit.v3HitPoint.z;
			result.hitNormalX = staticHit.v3HitNormal.x;
			result.hitNormalY = staticHit.v3HitNormal.y;
			result.hitNormalZ = staticHit.v3HitNormal.z;
		}

		// ── 4. 전체 클라이언트에 결과 브로드캐스트 ──────────────────────────
		for (auto& cl : clients)
			if (cl.m_is_connected)
				cl.send_shoot_result(result);
		break;
	}
	case C2S_PLAYER_RELOAD: {
		//std::cout << "[Reload] Player[" << m_id << "] requested reload\n";
		for (auto& cl : clients) {
			if (cl.m_is_connected) {
				cl.send_player_reload(m_id);
			}
		}
	}
	break;
	case C2S_PLAYER_MELEE: {
		C2S_PlayerMelee* packet = reinterpret_cast<C2S_PlayerMelee*>(p);

		Vector3 v3Origin{ packet->originX, packet->originY, packet->originZ };
		Vector3 v3Dir{ packet->dirX, packet->dirY, packet->dirZ };
		v3Dir.Normalize();

		constexpr float fRange  = 200.f;
		constexpr float fDamage = 50.f;

		// 단일 레이 — 전방 사거리 내 가장 가까운 좀비 1마리
		int   nHitZombieId = -1;
		float fHitDist     = fRange;
		g_ZombieManager.RayTestZombies(v3Origin, v3Dir, fRange, nHitZombieId, fHitDist);

		// 근접공격 모션을 전체 클라이언트에 브로드캐스트 (리모트 애니메이션)
		for (auto& cl : clients)
			if (cl.m_is_connected)
				cl.send_player_melee(m_id);

		// 히트 시 데미지 적용 + 피 이펙트 브로드캐스트
		if (nHitZombieId >= 0)
		{
			g_ZombieManager.ApplyDamageToZombie(nHitZombieId, fDamage);
			Vector3 v3Hit = v3Origin + v3Dir * fHitDist;
			for (auto& cl : clients)
				if (cl.m_is_connected)
					cl.send_melee_hit(m_id, nHitZombieId, fDamage, v3Hit);
		}
	}
	break;
	case C2S_PLAYER_WEAPON: {
		C2S_PlayerWeapon* packet = reinterpret_cast<C2S_PlayerWeapon*>(p);
		m_weaponType = packet->weaponType;   // late-join 동기화용으로 저장

		// 무기 교체를 전체 클라이언트에 브로드캐스트 (본인 포함; 클라가 본인 필터)
		for (auto& cl : clients)
			if (cl.m_is_connected)
				cl.send_player_weapon(m_id, m_weaponType);
	}
	break;
	case C2S_CHAT: {
		C2S_Chat* packet = reinterpret_cast<C2S_Chat*>(p);
		// 메시지 널 종단 보장
		char msg[MAX_CHAT_LEN];
		memcpy_s(msg, sizeof(msg), packet->message, MAX_CHAT_LEN);
		msg[MAX_CHAT_LEN - 1] = '\0';

		std::cout << "[Chat] Player[" << m_id << "] " << m_username << ": " << msg << std::endl;

		// 모든 접속 클라이언트(보낸 사람 포함)에게 브로드캐스트
		for (auto& cl : clients)
			if (cl.m_is_connected)
				cl.send_chat(m_id, m_username, msg);
	}
	break;
	default:
		std::cout << "Unknown packet type received from player[" << m_id << "].\n";
		return false;
	}
	return true;
}

void Session::send_player_melee(int attacker_id)
{
	S2C_PlayerMelee packet;
	packet.size = sizeof(S2C_PlayerMelee);
	packet.type = S2C_PLAYER_MELEE;
	packet.attackerPlayerId = attacker_id;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_melee_hit(int attacker_id, int zombie_id, float damage, const Vector3& v3Hit)
{
	S2C_MeleeHit packet;
	packet.size = sizeof(S2C_MeleeHit);
	packet.type = S2C_MELEE_HIT;
	packet.attackerPlayerId = attacker_id;
	packet.zombieId = zombie_id;
	packet.damage = damage;
	packet.hitX = v3Hit.x;
	packet.hitY = v3Hit.y;
	packet.hitZ = v3Hit.z;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_player_weapon(int player_id, unsigned char weapon_type)
{
	S2C_PlayerWeapon packet;
	packet.size = sizeof(S2C_PlayerWeapon);
	packet.type = S2C_PLAYER_WEAPON;
	packet.playerId = player_id;
	packet.weaponType = weapon_type;
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_chat(int sender_id, const char* username, const char* message)
{
	S2C_Chat packet;
	packet.size = sizeof(S2C_Chat);
	packet.type = S2C_CHAT;
	packet.playerId = sender_id;
	strncpy_s(packet.username, username, MAX_NAME_LEN - 1);
	strncpy_s(packet.message, message, MAX_CHAT_LEN - 1);
	do_send(packet.size, reinterpret_cast<char*>(&packet));
}

void Session::send_spawn_zombie(int nZombieId, const Vector3& v3Pos)
{
	S2C_SpawnZombie p;
	p.size = sizeof(S2C_SpawnZombie);
	p.type = S2C_SPAWN_ZOMBIE;
	p.zombieId = nZombieId;
	p.x = v3Pos.x;
	p.y = v3Pos.y;
	p.z = v3Pos.z;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void Session::send_despawn_zombie(int nZombieId)
{
	S2C_DespawnZombie p;
	p.size = sizeof(S2C_DespawnZombie);
	p.type = S2C_DESPAWN_ZOMBIE;
	p.zombieId = nZombieId;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void Session::send_zombie_state(int nZombieId, float x, float z, float yaw, float waypointX, float waypointZ, ZombieBehaviorState state)
{
	S2C_ZombieState p;
	p.size = sizeof(S2C_ZombieState);
	p.type = S2C_ZOMBIE_STATE;
	p.zombieId = nZombieId;
	p.x = x;
	p.z = z;
	p.yaw = yaw;
	p.waypointX = waypointX;
	p.waypointZ = waypointZ;
	p.behaviorState = state;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void Session::send_shoot_result(const S2C_ShootResult& result)
{
	do_send(result.size, const_cast<char*>(reinterpret_cast<const char*>(&result)));
}

void Session::send_zombie_attack(int nZombieId, int nTargetPlayerId, float fDamage)
{
	S2C_ZombieAttack p;
	p.size = sizeof(S2C_ZombieAttack);
	p.type = S2C_ZOMBIE_ATTACK;
	p.zombieId = nZombieId;
	p.targetPlayerId = nTargetPlayerId;
	p.damage = fDamage;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void Session::send_game_event(int event_id, const Vector3& v3Pos, float fTargetValue, float fDuration)
{
	S2C_GameEvent p;
	p.size = sizeof(S2C_GameEvent);
	p.type = S2C_GAME_EVENT;
	p.eventId = event_id;
	p.x = v3Pos.x;
	p.y = v3Pos.y;
	p.z = v3Pos.z;
	p.fTargetValue = fTargetValue;
	p.fDuration = fDuration;
	do_send(p.size, reinterpret_cast<char*>(&p));
}
