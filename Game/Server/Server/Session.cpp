#include "pch.h"
#include "Session.h"
#include "ZombieManager.h"

extern ZombieManager                g_ZombieManager;
extern std::unordered_map<int, Vector3> g_PlayerPositions;
extern std::mutex                   g_Mutex;

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

		float x = m_transform.m[3][0];
		float y = m_transform.m[3][1];
		float z = m_transform.m[3][2];

		std::cout << "Player[" << m_id << "] Transform Received - Pos(X: " << x << ", Y: " << y << ", Z: " << z << ")\n";

		send_transform_packet(m_id);
	}
					  break;
	default:
		std::cout << "Unknown packet type received from player[" << m_id << "].\n";
		return false;
	}
	return true;
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
