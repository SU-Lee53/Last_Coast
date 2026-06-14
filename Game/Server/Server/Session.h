#pragma once
#include "pch.h"
#include "Room.h"
#include "protocol.h"

class Session;

extern std::array<Session, MAX_PLAYERS> clients;

class EXP_OVER {
public:
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
		memset(&m_transform, 0, sizeof(m_transform));
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
	void		do_send(int num_bytes, char* mess);

	void		send_avatar_info();
	void		send_transform_packet(int mover);
	void		send_add_player(int player_id);
	void		send_login_success();
	void		send_remove_player(int player_id);
	bool		process_packet(unsigned char* p);

	void send_spawn_zombie(int nZombieId, const Vector3& v3Pos);
	void send_despawn_zombie(int nZombieId);
	void send_zombie_state(int nZombieId, float x, float z, float yaw, float waypointX, float waypointZ, ZombieBehaviorState state);
	void send_zombie_attack(int nZombieId, int nTargetPlayerId, float fDamage);
	void send_shoot_result(const S2C_ShootResult& result);
	void send_player_reload(int player_id);
	void send_player_melee(int attacker_id);
	void send_melee_hit(int attacker_id, int zombie_id, float damage, const Vector3& v3Hit);
	void send_chat(int sender_id, const char* username, const char* message);
	void send_player_weapon(int player_id, unsigned char weapon_type);

public:
	SOCKET		m_client;
	EXP_OVER	m_recv_over;
	Room*		m_room;
	int			m_id;
	std::atomic<bool> m_is_connected;
	int			m_prev_recv;
	char		m_username[MAX_NAME_LEN];
	TransformData m_transform;
	bool          m_bRunning = false;
	bool          m_bAiming = false;
	float         m_fAimPitch = 0.f;
	unsigned char m_weaponType = 3;   // 기본 PISTOL (WEAPON_TYPE::PISTOL == 3)
};
