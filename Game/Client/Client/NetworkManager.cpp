#include "pch.h"
#include "NetworkManager.h"
#include "Packets.h"

HANDLE NetworkManager::g_hNetworkThread = nullptr;
ConnectState m_connectState = ConnectState::None;

NetworkManager::~NetworkManager()
{
	Disconnect();
}

void NetworkManager::Initialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		__debugbreak();
		PostQuitMessage(0);
	}

	m_hClientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	if (m_hClientSocket == INVALID_SOCKET)
	{
		__debugbreak();
		PostQuitMessage(0);
	}
}

void NetworkManager::ConnectToServer()
{
	ImGui::Begin("NetworkManager::ConnectToServer()");
	{
		const char* cstrConnectionStatus = m_bConnected ? "Connected" : "Not connected yet";
		ImGui::Text(cstrConnectionStatus);
		ImGui::InputText("Server IP", m_cstrServerIP, IM_ARRAYSIZE(m_cstrServerIP));

		if (ImGui::Button("Try Connect")) {
			sockaddr_in serveraddr;
			memset(&serveraddr, 0, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(SERVERPORT);
			inet_pton(AF_INET, m_cstrServerIP, &serveraddr.sin_addr);
			int retval = WSAConnect(m_hClientSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr), 0, 0, 0, 0);

			if (retval == SOCKET_ERROR)
			{
				int err = WSAGetLastError();

				if (err == WSAEWOULDBLOCK)
				{
					m_connectState = ConnectState::Connecting;
					m_strErrorLog = "Connecting...";
				}
				else
				{
					m_connectState = ConnectState::Failed;
					m_strErrorLog = err_display("connect()");
				}
			}
			else
			{
				m_connectState = ConnectState::Connected;
				m_bConnected = true;
				g_hNetworkThread = CreateThread(NULL, 0, ProcessNetwork, this, 0, NULL);
			}
		}
		ImGui::Text(m_strErrorLog.c_str());

		if (ImGui::Button("Offline Mode")) {
			m_bConnected = true;
			m_bGameBegin = true;
		}

		if (m_bConnected) {
			ImGui::Text("Wait for game start...");
		}

		if (m_connectState == ConnectState::Connecting)
		{
			fd_set writeSet;
			FD_ZERO(&writeSet);
			FD_SET(m_hClientSocket, &writeSet);

			timeval timeout{};
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;

			int ret = select(0, nullptr, &writeSet, nullptr, &timeout);

			if (ret > 0)
			{
				int err = 0;
				int len = sizeof(err);

				getsockopt(m_hClientSocket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

				if (err == 0)
				{
					// 진짜 연결 성공
					m_connectState = ConnectState::Connected;
					m_bConnected = true;
					m_strErrorLog = "Connected!";

					g_hNetworkThread = CreateThread(NULL, 0, ProcessNetwork, this, 0, NULL);
				}
				else
				{
					// 연결 실패
					m_connectState = ConnectState::Failed;
					m_strErrorLog = "Connect Failed";
				}
			}
		}

		switch (m_connectState)
		{
		case ConnectState::None:
			ImGui::Text("Not connected");
			break;
		case ConnectState::Connecting:
			ImGui::Text("Connecting...");
			break;
		case ConnectState::Connected:
			ImGui::Text("Connected");
			break;
		case ConnectState::Failed:
			ImGui::Text("Failed");
			break;
		}
	}
	ImGui::End();
}

void NetworkManager::Disconnect()
{
	m_bConnected = false;
	CloseHandle(g_hNetworkThread);
	closesocket(m_hClientSocket);
	WSACleanup();
}

struct SendContext {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	char buffer[256];
};

void NetworkManager::SendPacket(void* packet, int size)
{
	if (!m_bConnected) return;

	SendContext* ctx = new SendContext();
	ZeroMemory(&ctx->over, sizeof(WSAOVERLAPPED));
	memcpy(ctx->buffer, packet, size);
	ctx->wsabuf.buf = ctx->buffer;
	ctx->wsabuf.len = size;

	int ret = WSASend(m_hClientSocket, &ctx->wsabuf, 1, nullptr, 0, &ctx->over, send_callback);
	if (ret == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING) {
			char szError[256];
			sprintf_s(szError, "WSASend Failed: %d\n", err);
			OutputDebugStringA(szError);
			delete ctx;
		}
	}
}

void NetworkManager::ReceiveData()
{
	DWORD flags = 0;

	m_wsabuf.buf = m_Buffer;
	m_wsabuf.len = BUF_SIZE;

	ZeroMemory(&m_over, sizeof(m_over));

	int ret = WSARecv(m_hClientSocket, &m_wsabuf, 1, nullptr, &flags, &m_over, recv_callback);

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			printf("WSARecv error: %d\n", err);
		}
	}
}

void NetworkManager::send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	// WSASend를 비동기로 호출했을 때 완료 처리
	SendContext* ctx = reinterpret_cast<SendContext*>(over);
	delete ctx;
}

void NetworkManager::recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
{
	NetworkManager* N = NetworkManager::GetInstance();
	
	if (num_bytes == 0 || err != 0) {
		N->Disconnect();
		return;
	}

	unsigned char* p = reinterpret_cast<unsigned char*>(N->m_Buffer);
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);

	switch(type) {
		case S2C_TRANSFORM: {
			S2C_Transform* pkt = reinterpret_cast<S2C_Transform*>(p);
			// TODO: 해당 플레이어의 Transform을 찾아 pkt->transform 행렬 적용
			// 예: Player* remotePlayer = FindPlayer(pkt->playerId);
			// remotePlayer->GetTransform()->SetWorldMatrix(Matrix(reinterpret_cast<float*>(pkt->transform.m)));
			break;
		}
		// ... 다른 패킷 처리 ...
	}

	// 다시 수신 대기 (핑퐁 제거, 계속해서 Recv만 돌림)
	N->m_over = *over;
	N->ReceiveData();
}

DWORD WINAPI NetworkManager::ProcessNetwork(LPVOID arg)
{
	NetworkManager* self = reinterpret_cast<NetworkManager*>(arg);

	self->ReceiveData();

	while (self->m_bConnected)
		SleepEx(100, true);

	self->Disconnect();

	return 0;
}
