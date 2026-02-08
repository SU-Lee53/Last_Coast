#pragma once
#include "Session.h"

class NetworkManager
{
public:
	void Init();
	void WorkerThreadMain(HANDLE iocpHandle);
	void Accept();

	
private:

	WSAData m_wsaData;
	SOCKET m_listenSocket;
	SOCKADDR_IN m_serverAddr;
	std::vector<Session*> m_sessionManager;

};

