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

class NetworkManager {

	DECLARE_SINGLE(NetworkManager)
	~NetworkManager();

public:
	void					Initialize();
	void					ConnectToServer();
	void					Disconnect();

private:
	void					SendData();
	void					ReceiveData();

	static void CALLBACK	send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);
	static void CALLBACK	recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

public:
	bool					IsConnected() const { return m_bConnected; }
	bool					IsGameStarted() const { return m_bGameBegin; }
	bool					IsOffline() const { return m_bOfflineMode; }

	int						GetPlayerID() const { return m_nPlayerID; }

	const std::string&		GetErrorLog() { return m_strErrorLog; }

private:
	WSAOVERLAPPED* m_over;
	WSABUF					m_wsabuf;
	SOCKET					m_hClientSocket;
	char					m_cstrServerIP[16] = "127.0.0.1";
	bool					m_bConnected = false;
	std::string				m_strErrorLog;
	int						m_nPlayerID;

	static HANDLE			g_hNetworkThread;
	static DWORD WINAPI		ProcessNetwork(LPVOID arg);

	bool					m_bGameBegin = false;
	bool					m_bOfflineMode = true;
};
