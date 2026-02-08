#include "pch.h"
#include "NetworkManager.h"
#include "Session.h"
#include "ThreadManager.h"

void NetworkManager::Init()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0)
		return;

	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (m_listenSocket == INVALID_SOCKET)
		return;

	m_sessionManager.reserve(1000);
}

void NetworkManager::WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;

		BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == FALSE || bytesTransferred == 0)
		{
			// TODO : 연결 끊김
			continue;
		}

		// 여기서 switch를 이용해 어떤걸 할지 결정한다.
		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ);

		std::cout << "Recv Data IOCP = " << bytesTransferred << std::endl;



		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = RECV_BUF_SIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
	}
}


void NetworkManager::Accept()
{
	memset(&m_serverAddr, 0, sizeof(m_serverAddr));
	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	m_serverAddr.sin_port = htons(SERVERPORT);

	if (::bind(m_listenSocket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr)) == SOCKET_ERROR)
		return;

	if (::listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return;

	std::cout << "Accept" << std::endl;

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket = ::accept(m_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return;

		Session* session = new Session();
		session->socket = clientSocket;
		m_sessionManager.push_back(session);

		std::cout << "Client Connected !" << std::endl;

		// CP 생성
		HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		// WorkerThreads
		for (int32 i = 0; i < 5; ++i)
		{
			GThreadManager->Launch([=]() {WorkerThreadMain(iocpHandle); });
		}
		// 소켓을 CP에 등록 (관찰 대상이라는걸 알려준다.)
		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*Key*/(ULONG_PTR)session, 0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = RECV_BUF_SIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &overlappedEx->overlapped, NULL);
	}
}
