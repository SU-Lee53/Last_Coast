#pragma once
#define SERVERPORT 9000

class NetworkManager {

	DECLARE_SINGLE(NetworkManager)
	~NetworkManager();

public:
	void Initialize();
	//void ConnectToServer();

private:
	void Disconnect();

	//bool SendData();
	//bool ReceiveData();
	//void MakePacketToSend(ClientToServerPacket& packet ,const std::shared_ptr<Player> Player);

public:
	//void WritePacketData(const ClientToServerPacket& packet);
	//ServertoClientPlayerPacket GetReceivedPacketData() const;

	//ServertoClientRockPacket GetReceivedRockPacketData();
	bool IsConnected() const { return m_bConnected; }
	bool IsGameStarted() const { return m_bGameBegin; }
	bool IsOffline() const { return m_bOfflineMode; }

	int GetPlayerID() const { return m_nPlayerID; }

	// Last Coast
	void WritePlayerState(const PlayerState& state);
	void StoreSnapshot(const ServerToClientPacket& packet);
	const ServerToClientPacket& GetSnapshot() const { return m_Snapshot; }

	const std::string& GetErrorLog() { return m_strErrorLog; }

private:
	WSADATA m_wsa;
	SOCKET m_hClientSocket;
	char m_cstrServerIP[16] = "127.0.0.1";
	bool m_bConnected = false;
	std::string m_strErrorLog;
	int m_nPlayerID;

	static HANDLE g_hNetworkThread;
	//static DWORD WINAPI ProcessNetwork(LPVOID arg);

	static CRITICAL_SECTION g_hCS;	// 이벤트로 순서제어 중이므로 사용 안될듯함 (아직 모름)
	static HANDLE g_hPlayerWritePacketEvent;
	static HANDLE g_hPacketReceivedEvent;

	ClientToServerPacket m_PacketToSend{};
	//ServertoClientPlayerPacket m_PacketReceived{};

	//ServertoClientRockPacket m_PacketRocksReceived{};

	bool m_bGameBegin = false;

	bool m_bOfflineMode = true;

	// Last Coast
	ClientToServerPacket m_SendPacket{};
	ServerToClientPacket m_Snapshot{};
};
