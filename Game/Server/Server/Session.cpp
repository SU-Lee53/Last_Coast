#include "Session.h"

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

bool Session::process_packet(unsigned char* p)
{
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
	switch (type) {
	case C2S_LOGIN: {
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
		strncpy_s(m_username, packet->username, MAX_NAME_LEN);
		std::cout << "Player[" << m_id << "] logged in as " << m_username << std::endl;
		send_avatar_info();
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
