#pragma once
#define SERVERPORT 9000
#include "ServerCore/Session.h"

class NetworkManager;

enum class ConnectState
{
	None,
	Connecting,
	Connected,
	Failed
};

struct OverEx {
	WSAOVERLAPPED over;
	NetworkManager* owner;
};

// 네트워크에서 수신한 원격 플레이어의 위치 스냅샷
struct NetSnapshot {
	Vector3 pos;
	float   time; // GetNetTimeSec() 기준 수신 시각
};

// 원격 플레이어 1명의 보간 상태
struct RemotePlayerState {
	static constexpr size_t MAX_SNAPSHOTS = 8;
	std::deque<NetSnapshot> snapshots;
	Vector3                 interpolatedPos = {};
	bool                    active = false;
};

// ── 좀비 네트워크 상태 ──────────────────────────────────────────────────────
// 서버에서 수신한 좀비 1마리의 최신 상태 (보간 없이 최신값 사용)
struct ZombieServerState {
	float               x = 0.f, z = 0.f;     // NavMesh XZ 권위적 위치 (cm)
	float               yaw = 0.f;
	float               waypointX = 0.f, waypointZ = 0.f;
	ZombieBehaviorState behaviorState = ZBS_Idle;
	float               receivedTime = 0.f;
	bool                valid = false;
};

// 좀비 스폰 이벤트 (메인 스레드에서 소비)
struct SpawnEvent {
	int     zombieId;
	Vector3 pos;
};

// 좀비 공격 히트 이벤트
struct AttackEvent {
	int   zombieId;
	int   targetPlayerId;
	float damage;
};

class NetworkManager {

	DECLARE_SINGLE(NetworkManager)
	~NetworkManager();

public:
	void					Initialize();
	void					ConnectToServer();
	void					Disconnect();

public:
	void					SendPacket(void* packet, int size);

	// 게임 루프에서 매 프레임 호출 — 보간 위치 갱신
	void					UpdateInterpolation();
	// 보간된 원격 플레이어 위치 반환. 해당 ID가 없으면 false
	bool					GetInterpolatedPosition(int playerId, Vector3& outPos) const;
	const std::unordered_map<int, RemotePlayerState>& GetRemotePlayers() const { return m_RemotePlayers; }

	// ── 플레이어 위치 전송 (Task 5) ──────────────────────────────────────────
	// 매 프레임 호출. 내부적으로 20Hz 로 스로틀링.
	void					SetLocalPlayerInfo(const Vector3& pos, float yaw);
	void					TrySendPlayerPosition();

	// ── 좀비 이벤트 소비 (Task 6/7/9) ────────────────────────────────────────
	// 수신된 이벤트를 메인 스레드에서 한 번에 가져온다 (swap-and-clear).
	std::vector<SpawnEvent>  ConsumeSpawnEvents();
	std::vector<int>         ConsumeDespawnEvents();
	std::vector<AttackEvent> ConsumeAttackEvents();

	// 최신 서버 좀비 상태 조회. 존재하지 않으면 false.
	bool					GetLatestZombieState(int zombieId, ZombieServerState& outState) const;

private:
	void					SendLoginPacket();
	void					SendData();
	void					ReceiveData();

	// 수신 버퍼에서 완성된 패킷을 추출해 처리
	void					ProcessPackets(int numBytes);
	void					ProcessSinglePacket(const char* data, int size);

	// 서버 좌표(short, 1unit = 1m) → 월드 좌표(cm)
	static float			ServerToWorld(short v) { return static_cast<float>(v) * 100.f; }
	// 앱 시작 기준 단조 시간(초) — 네트워크 스레드/게임 스레드 양쪽에서 사용
	static float			GetNetTimeSec();

	static void CALLBACK	send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
	static void CALLBACK	recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

public:
	bool					IsConnected() const { return m_bConnected; }
	bool					IsGameStarted() const { return m_bGameBegin; }
	bool					IsOffline() const { return m_bOfflineMode; }

	int						GetPlayerID() const { return m_nPlayerID; }

	const std::string&		GetErrorLog() { return m_strErrorLog; }

private:
	WSAOVERLAPPED			m_over = {};
	WSABUF					m_wsabuf;
	SOCKET					m_hClientSocket = INVALID_SOCKET;
	C2S_Move				m_SendMovePacket = {};     // SendData() overlapped 버퍼 (수명 보장)
	char					m_Buffer[BUF_SIZE];        // WSA DMA 수신 버퍼
	char					m_RecvBuf[BUF_SIZE * 4];   // 패킷 재조립 버퍼
	int						m_nRecvPending = 0;        // m_RecvBuf에 누적된 유효 바이트 수
	char					m_cstrServerIP[16] = "127.0.0.1";
	bool					m_bConnected = false;
	std::string				m_strErrorLog;
	int						m_nPlayerID = -1;

	// 원격 플레이어 상태. 네트워크 스레드(쓰기) / 게임 스레드(읽기) 공유 → m_Mutex 보호
	std::unordered_map<int, RemotePlayerState> m_RemotePlayers;
	mutable std::mutex		m_Mutex;

	// 보간 렌더 지연 (초). 이 값만큼 과거 시점을 보간해 항상 두 스냅샷 사이를 보장
	static constexpr float	INTERP_DELAY = 0.1f;

	static HANDLE			g_hNetworkThread;
	static DWORD WINAPI		ProcessNetwork(LPVOID arg);

	bool					m_bGameBegin = false;
	bool					m_bOfflineMode = true;

	// ── 플레이어 위치 전송 ────────────────────────────────────────────────────
	Vector3					m_v3LocalPlayerPos = {};
	float					m_fLocalPlayerYaw  = 0.f;
	float					m_fLastPosSendTime = 0.f;
	static constexpr float	POS_SEND_INTERVAL  = 1.f / 20.f; // 20Hz

	// ── 좀비 이벤트 큐 (lock-free concurrent 자료구조) ───────────────────────
	concurrency::concurrent_unordered_map<int, ZombieServerState> m_ZombieStates;
	concurrency::concurrent_queue<SpawnEvent>                     m_PendingSpawns;
	concurrency::concurrent_queue<int>                            m_PendingDespawns;
	concurrency::concurrent_queue<AttackEvent>                    m_PendingAttacks;
};
