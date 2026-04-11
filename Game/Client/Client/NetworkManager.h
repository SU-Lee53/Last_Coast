#pragma once
#define SERVERPORT 9000
#include "ServerCore/Session.h"
#include "../../../Server/Server/protocol.h"

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

class NetworkManager {

	DECLARE_SINGLE(NetworkManager)
	~NetworkManager();

public:
	void					Initialize();
	void					ConnectToServer();
	void					Disconnect();

	// 게임 루프에서 매 프레임 호출 — 보간 위치 갱신
	void					UpdateInterpolation();
	// 보간된 원격 플레이어 위치 반환. 해당 ID가 없으면 false
	bool					GetInterpolatedPosition(int playerId, Vector3& outPos) const;
	const std::unordered_map<int, RemotePlayerState>& GetRemotePlayers() const { return m_RemotePlayers; }

private:
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
	SOCKET					m_hClientSocket;
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
};
