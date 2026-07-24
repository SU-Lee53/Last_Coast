#include "pch.h"
#include "Session.h"
#include "GameWorld.h"
#include <concurrent_queue.h>

std::array<Session, MAX_PLAYERS> clients;

// 송신용 EXP_OVER 재사용 풀 (스레드 안전 lock-free 큐). 풀 크기는 동시 in-flight 송신 수만큼만 자란다.
static concurrency::concurrent_queue<EXP_OVER*> g_SendOverPool;

EXP_OVER* AcquireSendOver()
{
	EXP_OVER* o = nullptr;
	if (g_SendOverPool.try_pop(o)) {
		ZeroMemory(&o->m_over, sizeof(o->m_over));  // WSAOVERLAPPED는 재사용 전 반드시 0으로
		o->m_iotype  = IO_SEND;
		o->m_wsa.buf = o->m_buff;
		return o;
	}
	return new EXP_OVER(IO_SEND);
}

void ReleaseSendOver(EXP_OVER* o)
{
	if (o) 
		g_SendOverPool.push(o);
}

void Session::init(SOCKET s, int id, Room* room)
{
	m_client = s;
	m_id = id;
	m_is_connected = true;
	memset(&m_transform, 0, sizeof(m_transform));
	m_room = room;
	m_prev_recv = 0;

	// 슬롯 재사용 시 이전 세션의 로비 상태가 남지 않도록 리셋
	// (m_bReady 잔류 시 새 접속자가 레디한 것으로 카운트되어 전원 레디 오판)
	m_bReady = false;
	m_bRunning = false;
	m_bAiming = false;
	m_fAimPitch = 0.f;
	m_fHP = 100.f;
	m_bDead = false;
	m_fRespawnTimer = 0.f;
	m_v3SpawnPos = Vector3::Zero;
	m_bSpawnPosSet = false;
	m_weaponType = 3;      // 기본 PISTOL
	m_characterType = 0;   // 기본 캐릭터
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
	EXP_OVER* o = AcquireSendOver();   // 풀에서 재사용 (new 회피)
	o->m_wsa.len = num_bytes;
	memcpy(o->m_buff, mess, num_bytes);
	WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// C2S 패킷 디스패치 — 파싱만 담당. 로비/게임 로직은 PacketHandlers,
// 전투 판정(사격/근접)은 CombatSystem 이 처리한다.
// ─────────────────────────────────────────────────────────────────────────────
bool Session::process_packet(unsigned char* p)
{
	PacketHandlers& handlers = WORLD->GetHandlers();
	CombatSystem&   combat   = WORLD->GetCombat();

	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
	switch (type) {
	case C2S_LOGIN:
		handlers.Login(*this, *reinterpret_cast<C2S_Login*>(p));
		break;
	case C2S_REGISTER:
		handlers.Register(*this, *reinterpret_cast<C2S_Register*>(p));
		break;
	case C2S_TRANSFORM:
		handlers.Transform(*this, *reinterpret_cast<C2S_Transform*>(p));
		break;
	case C2S_PLAYER_SHOOT:
		combat.ResolveShoot(*this, *reinterpret_cast<C2S_PlayerShoot*>(p));
		break;
	case C2S_PLAYER_MELEE:
		combat.ResolveMelee(*this, *reinterpret_cast<C2S_PlayerMelee*>(p));
		break;
	case C2S_PLAYER_RELOAD:
		handlers.Reload(*this);
		break;
	case C2S_PLAYER_BANDAGE:
		handlers.Bandage(*this, *reinterpret_cast<C2S_PlayerBandage*>(p));
		break;
	case C2S_PLAYER_GRENADE:
		handlers.Grenade(*this, *reinterpret_cast<C2S_PlayerGrenade*>(p));
		break;
	case C2S_GRENADE_EXPLODE:
		handlers.GrenadeExplode(*this, *reinterpret_cast<C2S_GrenadeExplode*>(p));
		break;
	case C2S_PLAYER_WEAPON:
		handlers.Weapon(*this, *reinterpret_cast<C2S_PlayerWeapon*>(p));
		break;
	case C2S_PLAYER_CHARACTER:
		handlers.Character(*this, *reinterpret_cast<C2S_PlayerCharacter*>(p));
		break;
	case C2S_PLAYER_ESCAPE:
		handlers.Escape(*this);
		break;
	case C2S_CHAT:
		handlers.Chat(*this, *reinterpret_cast<C2S_Chat*>(p));
		break;
	case C2S_READY:
		handlers.Ready(*this, *reinterpret_cast<C2S_Ready*>(p));
		break;
	case C2S_GAME_START:
		handlers.GameStart(*this);
		break;
	case C2S_LOAD_COMPLETE:
		handlers.LoadComplete(*this);
		break;
	default:
		std::cout << "Unknown packet type received from player[" << m_id << "].\n";
		return false;
	}
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// S2C 전송 함수들
// ─────────────────────────────────────────────────────────────────────────────

void Session::send_add_player(int player_id)
{
	S2C_AddPlayer packet;
	packet.playerId = player_id;
	Session& pl = clients[player_id];
	memcpy(packet.username, pl.m_username, sizeof(packet.username));
	{
		std::lock_guard<std::mutex> lg(pl.m_state_lock); // 다른 세션의 transform 읽기 — tearing 방지
		packet.transform = pl.m_transform;
		packet.bRunning = pl.m_bRunning;
		packet.bAiming = pl.m_bAiming;
		packet.fAimPitch = pl.m_fAimPitch;
	}
	packet.weaponType = pl.m_weaponType;
	packet.bReady = pl.m_bReady;
	packet.characterType = pl.m_characterType;
	send_packet(S2C_ADD_PLAYER, packet);
}

void Session::send_avatar_info()
{
	S2C_AvatarInfo packet;
	packet.playerId = m_id;
	packet.transform = m_transform;
	send_packet(S2C_AVATAR_INFO, packet);
}

void Session::send_transform_packet(int mover)
{
	if (m_room == nullptr) return;

	S2C_Transform packet;
	packet.playerId = mover;
	{
		std::lock_guard<std::mutex> lg(clients[mover].m_state_lock);
		packet.transform = clients[mover].m_transform;
		packet.bRunning = clients[mover].m_bRunning;
		packet.bAiming = clients[mover].m_bAiming;
		packet.fAimPitch = clients[mover].m_fAimPitch;
	}

	for (int p : m_room->players) {
		if (p == -1) continue;
		if (p == mover) continue;
		clients[p].send_packet(S2C_TRANSFORM, packet);
	}
}

void Session::send_login_success()
{
	S2C_LoginResult packet;
	packet.success = true;
	strncpy_s(packet.message, "Login successful.", sizeof(packet.message));
	send_packet(S2C_LOGIN_RESULT, packet);
}

void Session::send_remove_player(int player_id)
{
	S2C_RemovePlayer packet;
	packet.playerId = player_id;
	send_packet(S2C_REMOVE_PLAYER, packet);
}

void Session::send_player_reload(int player_id)
{
	S2C_PlayerReload packet;
	packet.playerId = player_id;
	send_packet(S2C_PLAYER_RELOAD, packet);
}

void Session::send_player_melee(int attacker_id)
{
	S2C_PlayerMelee packet;
	packet.attackerPlayerId = attacker_id;
	send_packet(S2C_PLAYER_MELEE, packet);
}

void Session::send_melee_hit(int attacker_id, int zombie_id, float damage, const Vector3& v3Hit)
{
	S2C_MeleeHit packet;
	packet.attackerPlayerId = attacker_id;
	packet.zombieId = zombie_id;
	packet.damage = damage;
	packet.hitX = v3Hit.x;
	packet.hitY = v3Hit.y;
	packet.hitZ = v3Hit.z;
	send_packet(S2C_MELEE_HIT, packet);
}

void Session::send_player_weapon(int player_id, unsigned char weapon_type)
{
	S2C_PlayerWeapon packet;
	packet.playerId = player_id;
	packet.weaponType = weapon_type;
	send_packet(S2C_PLAYER_WEAPON, packet);
}

void Session::send_player_character(int player_id, unsigned char character_type)
{
	S2C_PlayerCharacter packet;
	packet.playerId = player_id;
	packet.characterType = character_type;
	send_packet(S2C_PLAYER_CHARACTER, packet);
}

void Session::send_chat(int sender_id, const std::string& username, const std::string& message)
{
	S2C_Chat packet;
	packet.playerId = sender_id;
	strncpy_s(packet.username, username.c_str(), MAX_NAME_LEN - 1);
	strncpy_s(packet.message, message.c_str(), MAX_CHAT_LEN - 1);
	send_packet(S2C_CHAT, packet);
}

void Session::send_spawn_zombie(int nZombieId, const Vector3& v3Pos)
{
	S2C_SpawnZombie p;
	p.zombieId = nZombieId;
	p.x = v3Pos.x;
	p.y = v3Pos.y;
	p.z = v3Pos.z;
	send_packet(S2C_SPAWN_ZOMBIE, p);
}

void Session::send_despawn_zombie(int nZombieId)
{
	S2C_DespawnZombie p;
	p.zombieId = nZombieId;
	send_packet(S2C_DESPAWN_ZOMBIE, p);
}

void Session::send_zombie_state(int nZombieId, float x, float z, float yaw, float waypointX, float waypointZ, ZombieBehaviorState state)
{
	S2C_ZombieState p;
	p.zombieId = nZombieId;
	p.x = x;
	p.z = z;
	p.yaw = yaw;
	p.waypointX = waypointX;
	p.waypointZ = waypointZ;
	p.behaviorState = state;
	send_packet(S2C_ZOMBIE_STATE, p);
}

void Session::send_zombie_state_batch(const ZombieStateEntry* entries, int count)
{
	if (count <= 0 || !entries) return;
	if (count > MAX_ZOMBIE_BATCH) count = MAX_ZOMBIE_BATCH;

	S2C_ZombieStateBatch p;
	p.type  = S2C_ZOMBIE_STATE_BATCH;
	p.count = static_cast<unsigned char>(count);
	for (int i = 0; i < count; ++i) p.entries[i] = entries[i];

	// 미사용 엔트리 제외하고 실제 크기만 전송 (size 는 unsigned char ≤ 255).
	const int nBytes = static_cast<int>(offsetof(S2C_ZombieStateBatch, entries))
		+ count * static_cast<int>(sizeof(ZombieStateEntry));
	p.size = static_cast<unsigned char>(nBytes);
	do_send(nBytes, reinterpret_cast<char*>(&p));
}

void Session::send_zombie_attack(int nZombieId, int nTargetPlayerId, float fDamage)
{
	S2C_ZombieAttack p;
	p.zombieId = nZombieId;
	p.targetPlayerId = nTargetPlayerId;
	p.damage = fDamage;
	send_packet(S2C_ZOMBIE_ATTACK, p);
}

void Session::send_game_event(int event_id, const Vector3& v3Pos, float fTargetValue, float fDuration, int preset_id)
{
	S2C_GameEvent p;
	p.eventId = event_id;
	p.x = v3Pos.x;
	p.y = v3Pos.y;
	p.z = v3Pos.z;
	p.fTargetValue = fTargetValue;
	p.fDuration = fDuration;
	p.presetId = preset_id;
	send_packet(S2C_GAME_EVENT, p);
}

void Session::send_ready_state(int player_id, bool bReady)
{
	S2C_ReadyState p;
	p.playerId = player_id;
	p.bReady = bReady;
	send_packet(S2C_READY_STATE, p);
}

void Session::send_game_start()
{
	S2C_GameStart p;
	send_packet(S2C_GAME_START, p);
}

void Session::send_game_begin()
{
	S2C_GameBegin p;
	send_packet(S2C_GAME_BEGIN, p);
}

void Session::send_host_change(int host_id)
{
	S2C_HostChange p;
	p.hostPlayerId = host_id;
	send_packet(S2C_HOST_CHANGE, p);
}

void Session::send_escape_state(unsigned char phase, float remain_seconds)
{
	S2C_EscapeState p;
	p.phase = phase;
	p.fRemainSeconds = remain_seconds;
	send_packet(S2C_ESCAPE_STATE, p);
}

void Session::send_game_end()
{
	S2C_GameEnd p;
	send_packet(S2C_GAME_END, p);
}

void Session::send_player_death(int player_id, float respawn_seconds)
{
	S2C_PlayerDeath p;
	p.playerId        = player_id;
	p.fRespawnSeconds = respawn_seconds;
	send_packet(S2C_PLAYER_DEATH, p);
}

void Session::send_player_respawn(int player_id, const Vector3& v3Pos)
{
	S2C_PlayerRespawn p;
	p.playerId = player_id;
	p.x = v3Pos.x;
	p.y = v3Pos.y;
	p.z = v3Pos.z;
	send_packet(S2C_PLAYER_RESPAWN, p);
}

void Session::send_player_bandage(int player_id, int target_id, unsigned char state)
{
	S2C_PlayerBandage p;
	p.playerId       = player_id;
	p.targetPlayerId = target_id;
	p.state          = state;
	send_packet(S2C_PLAYER_BANDAGE, p);
}

void Session::send_player_heal(int target_id, int healer_id, float new_hp)
{
	S2C_PlayerHeal p;
	p.targetPlayerId = target_id;
	p.healerPlayerId = healer_id;
	p.fNewHP         = new_hp;
	send_packet(S2C_PLAYER_HEAL, p);
}

void Session::send_player_grenade(int player_id, const C2S_PlayerGrenade& pkt)
{
	S2C_PlayerGrenade p;
	p.playerId = player_id;
	p.state    = pkt.state;
	p.x  = pkt.x;  p.y  = pkt.y;  p.z  = pkt.z;
	p.vx = pkt.vx; p.vy = pkt.vy; p.vz = pkt.vz;
	send_packet(S2C_PLAYER_GRENADE, p);
}

void Session::send_grenade_hit(int attacker_id, int zombie_id, float damage)
{
	S2C_GrenadeHit p;
	p.attackerPlayerId = attacker_id;
	p.zombieId         = zombie_id;
	p.damage           = damage;
	send_packet(S2C_GRENADE_HIT, p);
}
