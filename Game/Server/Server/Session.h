#pragma once
#include "pch.h"
#include "Room.h"

class Session;

extern std::array<Session, MAX_PLAYERS> clients;

class EXP_OVER {

	EXP_OVER()
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}
	EXP_OVER(IOType iot) : m_iotype(iot)
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}

public:
	WSAOVERLAPPED		m_over;
	IOType				m_iotype;
	WSABUF				m_wsa;
	SOCKET				m_client_socket;
	char				m_buff[BUF_SIZE];
};

class Session {
public:
	Session() {
		m_is_connected = false;
		m_id = 999;
		m_client = INVALID_SOCKET;
		m_recv_over.m_iotype = IO_RECV;
		m_x = 0; 		m_y = 0;
		m_prev_recv = 0;
	}
	~Session()
	{
		if (m_is_connected)
			closesocket(m_client);
	}

public:
	void		init(SOCKET s, int id, Room* room);
	void		do_recv();									
	void		do_send(int num_bytes, char* mess)
	{
		EXP_OVER* o = new EXP_OVER(IO_SEND);
		o->m_wsa.len = num_bytes;
		memcpy(o->m_buff, mess, num_bytes);
		WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
	}
	void		send_avatar_info()
	{
		S2C_AvatarInfo packet;
		packet.size = sizeof(S2C_AvatarInfo);
		packet.type = S2C_AVATAR_INFO;
		packet.playerId = m_id;
		packet.x = m_x;
		packet.y = m_y;
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
	void		send_move_packet(int mover);
	void		send_add_player(int player_id);
	void		send_login_success()
	{
		S2C_LoginResult packet;
		packet.size = sizeof(S2C_LoginResult);
		packet.type = S2C_LOGIN_RESULT;
		packet.success = true;
		strncpy_s(packet.message, "Login successful.", sizeof(packet.message));
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
	void		send_remove_player(int player_id)
	{
		S2C_RemovePlayer packet;
		packet.size = sizeof(S2C_RemovePlayer);
		packet.type = S2C_REMOVE_PLAYER;
		packet.playerId = player_id;
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}
	void		process_packet(unsigned char* p);

public:
	SOCKET		m_client;
	EXP_OVER	m_recv_over;
	Room*		m_room;
	int			m_id;
	bool		m_is_connected;
	int			m_prev_recv;
	char		m_username[MAX_NAME_LEN];
	short		m_x, m_y;
};
