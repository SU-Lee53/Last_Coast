#include "Session.h"

void Session::init(SOCKET s, int id, Room* room)
{
	m_client = s;
	m_id = id;
	m_is_connected = true;
	m_x = 0;
	m_y = 0;
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
	packet.x = pl.m_x;
	packet.y = pl.m_y;
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
	case C2S_MOVE: {
		C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
		DIRECTION dir = packet->dir;
		// TODO : Move 로직
		switch (dir) {
		case UP: m_y++;
			break;
		case DOWN: m_y--;
			break;
		case LEFT: m_x--;
			break;
		case RIGHT: m_x++;
			break;
		}
		std::cout << "Player[" << m_id << "] moved to (" << m_x << ", " << m_y << ")\n";
		send_move_packet(m_id);
	}
				 break;
	default:
		std::cout << "Unknown packet type received from player[" << m_id << "].\n";
		return false;
	}
	return true;
}

void Session::send_move_packet(int mover)
{
	if (m_room == nullptr) return;

	S2C_MovePlayer packet;
	packet.size = sizeof(S2C_MovePlayer);
	packet.type = S2C_MOVE_PLAYER;
	packet.playerId = mover;
	packet.x = clients[mover].m_x;
	packet.y = clients[mover].m_y;

	for (int p : m_room->players) {
		if (p == -1) continue;
		clients[p].do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
}
