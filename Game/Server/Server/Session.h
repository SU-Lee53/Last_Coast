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

	// S2C 공통 전송 — size/type 세팅 + do_send. 나머지 필드는 호출부가 채워 전달.
	// 예외: 가변 길이 패킷(send_zombie_state_batch)은 do_send 직접 사용.
	template<typename T>
	void send_packet(PACKET_TYPE type, T& packet)
	{
		packet.size = static_cast<unsigned char>(sizeof(T));
		packet.type = type;
		do_send(packet.size, reinterpret_cast<char*>(&packet));
	}

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
	void send_player_reload(int player_id);
	void send_player_melee(int attacker_id);
	void send_melee_hit(int attacker_id, int zombie_id, float damage, const Vector3& v3Hit);
	void send_chat(int sender_id, const std::string& username, const std::string& message);
	void send_player_weapon(int player_id, unsigned char weapon_type);
	void send_player_character(int player_id, unsigned char character_type);
	void send_game_event(int event_id, const Vector3& v3Pos, float fTargetValue, float fDuration, int preset_id = 0);
	void send_ready_state(int player_id, bool bReady);
	void send_game_start();
	void send_host_change(int host_id);
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
	// m_transform + 이동 플래그 보호 — 워커 스레드가 쓰고(C2S_TRANSFORM)
	// 틱 스레드/다른 세션 핸들러가 읽음(위치 스냅샷, send_add_player 등). 행렬 tearing 방지.
	std::mutex	m_state_lock;
	TransformData m_transform;
	bool          m_bRunning = false;
	bool          m_bAiming = false;
	float         m_fAimPitch = 0.f;
	unsigned char m_weaponType = 3;   // 기본 PISTOL (WEAPON_TYPE::PISTOL == 3)
	bool          m_bReady = false;
	unsigned char m_characterType = 0; // 기본 캐릭터 모델 인덱스 (g_strCharacterNames[0])
};

// ─────────────────────────────────────────────────────────────────────────────
// 전체 클라이언트 브로드캐스트 헬퍼 (워커/게임 틱 스레드 공용)
// ─────────────────────────────────────────────────────────────────────────────
template<typename Fn>
void BroadcastAll(Fn fn)
{
	for (auto& cl : clients)
		if (cl.m_is_connected)
			fn(cl);
}
