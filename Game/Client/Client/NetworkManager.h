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

struct PlayerJoinEvent {
	int           playerId;
	TransformData initialTransform;
	bool          bRunning;
	bool          bAiming;
	float         fAimPitch;
	unsigned char weaponType;
	std::string   username;
	bool          bReady;
	unsigned char characterType;
};

// 레디 상태 변경 이벤트
struct ReadyStateEvent {
	int  playerId;
	bool bReady;
};

// 무기 교체 이벤트 (리모트 플레이어 무기 반영)
struct WeaponChangeEvent {
	int           playerId;
	unsigned char weaponType;
};

// 캐릭터 모델 변경 이벤트 (리모트 플레이어 모델 반영)
struct CharacterChangeEvent {
	int           playerId;
	unsigned char characterType;
};

struct PlayerTransformEvent {
	int           playerId;
	TransformData transform;
	bool          bRunning;
	bool          bAiming;
	float         fAimPitch;
	float         fRecvTime;  // 네트워크 스레드 도착 시각 (GetNetTimeSec 기준) — 보간 시간축
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

// 사격 결과 이벤트
struct ShootResultEvent {
	int           shooterPlayerId;
	unsigned char bHit;              // 0=miss, 1=static, 2=zombie
	Vector3       v3HitPoint;
	Vector3       v3HitNormal;
	int           hitZombieId;       // -1 = 미히트
	float         damage;
	Vector3       v3MuzzlePos;       // 발사자 총구 위치
	Vector3       v3ShootDir;        // 발사 방향
};

// 근접공격 좀비 히트 이벤트
struct MeleeHitEvent {
	int     attackerPlayerId;
	int     zombieId;
	float   damage;
	Vector3 v3HitPoint;
};

// 채팅 메시지 이벤트
struct ChatMessageEvent {
	int         playerId;
	std::string username;
	std::string message;
};

// 플레이어 사망 이벤트 (서버 권위 — 본인이면 관전 진입, 리모트면 관전 후보 제외)
struct PlayerDeathEvent {
	int   playerId;
	float fRespawnSeconds;
};

// 플레이어 부활 이벤트 (본인이면 관전 해제 + HP 회복 + 위치 이동)
struct PlayerRespawnEvent {
	int     playerId;
	Vector3 pos;
};

// 서버 스크립트 게임 이벤트 (폭파/포스트FX 등). eventId = GameEventId.
struct GameEventMsg {
	int     eventId;
	Vector3 pos;
	float   fTargetValue;
	float   fDuration;
	int     presetId;     // EnvironmentPresetId (GE_ENVIRONMENT 전용)
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

	// ── 사격 송수신 ──────────────────────────────────────────────────────────
	void SendPlayerShoot(const Vector3& v3Origin, const Vector3& v3Direction, const Vector3& v3MuzzlePos, float damage);
	std::vector<ShootResultEvent> ConsumeShootResults();

	void SendPlayerReload();
	std::vector<int> ConsumePlayerReloads();

	// ── 근접공격 송수신 ────────────────────────────────────────────────────────
	void SendPlayerMelee(const Vector3& v3Origin, const Vector3& v3Direction);
	std::vector<int>           ConsumePlayerMelees(); // 근접공격 모션 (attackerPlayerId)
	std::vector<MeleeHitEvent> ConsumeMeleeHits();    // 좀비 히트

	// ── 무기 교체 송수신 ────────────────────────────────────────────────────────
	void SendPlayerWeapon(unsigned char weaponType);
	std::vector<WeaponChangeEvent> ConsumePlayerWeapons();

	// ── 캐릭터 모델 송수신 ──────────────────────────────────────────────────────
	void SendPlayerCharacter(unsigned char characterType);
	std::vector<CharacterChangeEvent> ConsumePlayerCharacters();

	// ── 채팅 송수신 ────────────────────────────────────────────────────────────
	void SendChat(const std::string& message);
	std::vector<ChatMessageEvent> ConsumeChatMessages();

	// ── 서버 게임 이벤트 소비 ──────────────────────────────────────────────────
	std::vector<GameEventMsg> ConsumeGameEvents();
	// ↳ 패킷 보내기
	void SendReady(bool bReady);

	// 어떤 플레이어의 레디 상태 변경을 수신
	std::vector<ReadyStateEvent> ConsumeReadyStates();
	// 게임 시작 신호 (서버 발송) — 씬 로딩 개시
	bool                         ConsumeGameStart();
	// 게임씬 로딩 완료 통지 — 서버가 방 전원 수신 확인 후 S2C_GAME_BEGIN 브로드캐스트
	void                         SendLoadComplete();
	// 전원 로딩 완료 — 게임플레이 동시 시작 신호
	bool                         ConsumeGameBegin();
	// 방장 전용: 게임 시작 요청 (서버가 전원 레디 검증 후 S2C_GAME_START 브로드캐스트)
	void                         SendGameStart();

	// ── 탈출 시퀀스 (서버 권위) ────────────────────────────────────────────────
	// 서버가 보낸 최신 탈출 상태가 있으면 true + phase(0=서바이벌,1=탈출가능)/남은초 반환.
	bool  GetEscapeState(unsigned char& outPhase, float& outRemainSeconds) const;
	// 탈출 키 입력을 서버에 전송 (탈출 가능 상태에서 누구든)
	void  SendPlayerEscape();
	// 게임 종료(탈출 성공) 신호 소비 (1회)
	bool  ConsumeGameEnd();

	// ── 플레이어 사망/부활 소비 (서버 권위) ─────────────────────────────────────
	std::vector<PlayerDeathEvent>     ConsumePlayerDeaths();
	std::vector<PlayerRespawnEvent>   ConsumePlayerRespawns();

	// ── 플레이어 이벤트 소비 (Task: Remote Player Sync) ─────────────────────────
	std::vector<PlayerJoinEvent>      ConsumePlayerJoins();
	std::vector<int>                  ConsumePlayerLeaves();
	std::vector<PlayerTransformEvent> ConsumePlayerTransforms();

	// 현재 방에 접속한 모든 플레이어 스냅샷 (큐 소비와 무관하게 유지됨).
	// 씬 전환(로비→게임) 시 큐가 이미 비어도 리모트 플레이어를 재구성하기 위함.
	std::vector<PlayerJoinEvent>      GetRoomPlayersSnapshot();

	// 최신 서버 좀비 상태 조회. 존재하지 않으면 false.
	bool					GetLatestZombieState(int zombieId, ZombieServerState& outState) const;

	// 앱 시작 기준 단조 시간(초) — 네트워크 스레드/게임 스레드 양쪽에서 사용
	static float			GetNetTimeSec();

public:
	void					SendLogin(const std::string& id, const std::string& pw);
	void					SendRegister(const std::string& id, const std::string& pw);

	// ── 결과 상태 확인용 ───────────────────────────────────────────────────────
	int						m_nLoginState = 0; // 0: None, 1: Success, -1: Failed
	int						m_nRegisterState = 0; // 0: None, 1: Success, -1: Failed

private:
	void					SendData();
	void					ReceiveData();

	// 수신 버퍼에서 완성된 패킷을 추출해 처리
	void					ProcessPackets(int numBytes);
	void					ProcessSinglePacket(const char* data, int size);

	// 서버 좌표(short, 1unit = 1m) → 월드 좌표(cm)
	static float			ServerToWorld(short v) { return static_cast<float>(v) * 100.f; }

	static void CALLBACK	send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
	static void CALLBACK	recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

public:
	bool					IsConnected() const { return m_bConnected; }
	bool					IsGameStarted() const { return m_bGameBegin; }
	bool					IsOffline() const { return m_bOfflineMode; }

	int						GetPlayerID() const { return m_nPlayerID; }

	// 방장(호스트) 플레이어 ID. 서버가 S2C_HOST_CHANGE 로 통지 (-1 = 미정)
	int						GetHostId() const { return m_nHostId.load(); }
	bool					IsHost() const { return m_nPlayerID >= 0 && m_nHostId.load() == m_nPlayerID; }

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
	volatile bool			m_bConnected = false;
	std::string				m_strErrorLog;
	int						m_nPlayerID = -1;
	std::atomic<int>		m_nHostId{ -1 };   // 네트워크 스레드 쓰기 / 게임 스레드 읽기

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
	concurrency::concurrent_queue<ShootResultEvent>               m_PendingShootResults;

	// ── 플레이어 이벤트 큐 ───────────────────────────────────────────────────
	concurrency::concurrent_queue<PlayerJoinEvent>                m_PendingPlayerJoins;
	concurrency::concurrent_queue<int>                            m_PendingPlayerLeaves;
	concurrency::concurrent_queue<PlayerTransformEvent>           m_PendingPlayerTransforms;
	concurrency::concurrent_queue<int>                            m_PendingPlayerReloads;
	concurrency::concurrent_queue<int>                            m_PendingPlayerMelees;
	concurrency::concurrent_queue<MeleeHitEvent>                  m_PendingMeleeHits;
	concurrency::concurrent_queue<ChatMessageEvent>               m_PendingChatMessages;
	concurrency::concurrent_queue<WeaponChangeEvent>             m_PendingPlayerWeapons;
	concurrency::concurrent_queue<CharacterChangeEvent>          m_PendingPlayerCharacters;
	concurrency::concurrent_queue<GameEventMsg>                  m_PendingGameEvents;
	concurrency::concurrent_queue<ReadyStateEvent>               m_PendingReadyStates;
	concurrency::concurrent_queue<PlayerDeathEvent>              m_PendingPlayerDeaths;
	concurrency::concurrent_queue<PlayerRespawnEvent>            m_PendingPlayerRespawns;
	std::atomic<bool>                                            m_bPendingGameStart{ false };
	std::atomic<bool>                                            m_bPendingGameBegin{ false };

	// 탈출 시퀀스 상태 (서버가 주기적으로 보냄 = 최신값만 유지) + 종료 신호
	std::atomic<int>                                            m_nEscapePhase{ -1 };   // -1=없음, 0=서바이벌, 1=탈출가능
	std::atomic<float>                                          m_fEscapeRemain{ 0.f }; // 남은 서바이벌 시간(초)
	std::atomic<bool>                                           m_bPendingGameEnd{ false };

	// 방 멤버 보존 맵 (큐 소비와 무관하게 유지). 씬 전환 후 리모트 재구성용. m_RoomPlayersMutex 보호
	std::unordered_map<int, PlayerJoinEvent>                     m_RoomPlayers;
	std::mutex                                                   m_RoomPlayersMutex;
};
