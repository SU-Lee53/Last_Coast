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

// 송신용 EXP_OVER 풀 (do_send 가 매 패킷마다 new/delete 하던 것을 재사용으로 대체 — 스레드 안전).
// Acquire: 풀에서 꺼내거나(없으면 new) 송신용으로 초기화. Release: IO_SEND 완료 시 풀에 반납.
EXP_OVER* AcquireSendOver();
void      ReleaseSendOver(EXP_OVER* o);

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
	void send_zombie_state_batch(const ZombieStateEntry* entries, int count);
	void send_zombie_attack(int nZombieId, int nTargetPlayerId, float fDamage);
	void send_shoot_result(const S2C_ShootResult& result);
	void send_player_reload(int player_id);
	void send_player_melee(int attacker_id);
	void send_melee_hit(int attacker_id, int zombie_id, float damage, const Vector3& v3Hit);
	void send_chat(int sender_id, const char* username, const char* message);
	void send_player_weapon(int player_id, unsigned char weapon_type);
	void send_player_character(int player_id, unsigned char character_type);
	void send_game_event(int event_id, const Vector3& v3Pos, float fTargetValue, float fDuration, int preset_id = 0);
	void send_ready_state(int player_id, bool bReady);
	void send_game_start();
	void send_escape_state(unsigned char phase, float remain_seconds);
	void send_game_end();

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
	bool          m_bReady = false;
	unsigned char m_characterType = 0; // 기본 캐릭터 모델 인덱스 (g_strCharacterNames[0])
};
