#include "pch.h"
#include "Network.h"
#include "Room.h"
#include "GameWorld.h"

namespace
{
	void error_display(const wchar_t* msg, int err_no)
	{
		WCHAR* lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);
		std::wcout << msg << L" === 에러 " << lpMsgBuf << std::endl;
		LocalFree(lpMsgBuf);
	}

	// 세션 배정 전(서버 꽉 참) 소켓에 직접 로그인 실패 응답 전송
	void send_login_fail(SOCKET client, const std::string& message)
	{
		S2C_LoginResult p;
		p.size = sizeof(S2C_LoginResult);
		p.type = S2C_LOGIN_RESULT;
		p.success = false;
		strncpy_s(p.message, message.c_str(), sizeof(p.message));
		EXP_OVER* o = AcquireSendOver();   // 풀에서 재사용 (new 회피)
		o->m_wsa.len = p.size;
		memcpy(o->m_buff, &p, p.size);
		WSASend(client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
	}
}

bool Network::Init(short port)
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(m_listenSocket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(m_listenSocket, SOMAXCONN);

	m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort((HANDLE)m_listenSocket, m_iocp, -1, 0);

	m_acceptOver.m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	AcceptEx(m_listenSocket, m_acceptOver.m_client_socket, m_acceptOver.m_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, &m_acceptOver.m_over);

	return true;
}

void Network::Shutdown()
{
	closesocket(m_listenSocket);
	m_listenSocket = INVALID_SOCKET;
	WSACleanup();
}

void Network::Disconnect(int id)
{
	Session& cl = clients[id];

	if (!cl.m_is_connected) return;

	// Room에서 제거
	if (cl.m_room != nullptr) {
		Room* room = cl.m_room;

		for (int other_id : room->players) {
			if (other_id == -1 || other_id == id) continue;
			clients[other_id].send_remove_player(id);
		}

		room->remove_player(id);
		cl.m_room = nullptr;

		// 방장이 나갔으면 remove_player 가 새 방장을 지정 → 남은 인원에게 통지
		for (int other_id : room->players) {
			if (other_id == -1) continue;
			clients[other_id].send_host_change(room->host_id);
		}

		// 로딩 대기 중 이탈 — 남은 인원이 전부 로딩 완료 상태면 그대로 시작
		// (안 하면 이탈자의 로딩 완료를 영원히 기다리며 전원이 멈춰 있는다)
		WORLD->GetHandlers().TryBeginGame(room);
	}

	closesocket(cl.m_client);
	cl.m_client = INVALID_SOCKET;
	cl.m_is_connected = false;
}

void Network::HandleAccept(EXP_OVER& over)
{
	int player_index = -1;
	// 빈 Session 슬롯 찾기
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (!clients[i].m_is_connected) {
			player_index = i;
			break;
		}
	}
	// 서버 인원 꽉 참
	if (player_index == -1) {
		send_login_fail(over.m_client_socket, "Server Full");
		closesocket(over.m_client_socket);
	}
	// 접속 허용
	else {
		// 방 할당 및 브로드캐스트는 로그인 성공 시 처리
		CreateIoCompletionPort((HANDLE)over.m_client_socket, m_iocp, player_index, 0);

		// Nagle 비활성화 — 작은 패킷 즉시 전송 (좀비 상태 패킷 지연 방지)
		BOOL bNoDelay = TRUE;
		setsockopt(over.m_client_socket, IPPROTO_TCP, TCP_NODELAY,
			reinterpret_cast<const char*>(&bNoDelay), sizeof(bNoDelay));

		clients[player_index].init(over.m_client_socket, player_index, nullptr);
		clients[player_index].do_recv();
	}

	// 다음 Accept 준비
	over.m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	ZeroMemory(&over.m_over, sizeof(over.m_over));
	AcceptEx(m_listenSocket, over.m_client_socket, over.m_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &over.m_over);
}

void Network::WorkerLoop()
{
	while (true) {
		DWORD num_bytes = 0;
		ULONG_PTR long_key = 0;
		LPOVERLAPPED over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(m_iocp, &num_bytes, &long_key, &over, INFINITE);
		int key = static_cast<int>(long_key);
		if (FALSE == ret) {
			// 클라이언트 강제 종료(RST) 등으로 pending I/O가 실패한 경우 —
			// 세션을 정리하고 송신 오버랩드는 풀에 반납해 누수 방지.
			error_display(L"GQCS Error", WSAGetLastError());
			if (over != nullptr) {
				EXP_OVER* failed = reinterpret_cast<EXP_OVER*>(over);
				if (failed->m_iotype == IO_RECV)
					Disconnect(key);
				else if (failed->m_iotype == IO_SEND)
					ReleaseSendOver(failed);
			}
			continue;
		}
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

		switch (exp_over->m_iotype) {
		case IO_ACCEPT:
			HandleAccept(*exp_over);
			break;

		case IO_RECV:
		{
			if (0 == num_bytes) {
				Disconnect(key);
				break;
			}

			Session& cl = clients[key];

			if (!cl.m_is_connected) {
				std::cout << "Session not found for client[" << key << "].\n";
				break;
			}

			unsigned char* p = reinterpret_cast<unsigned char*>(exp_over->m_buff);
			int data_size = num_bytes + cl.m_prev_recv;

			while (data_size > 0) {
				int packet_size = p[0];
				if (packet_size > data_size) break;

				if (false == cl.process_packet(p)) {
					Disconnect(key);
					break;
				}

				p += packet_size;
				data_size -= packet_size;
			}

			if (data_size > 0)
				memmove(cl.m_recv_over.m_buff, p, data_size);

			cl.m_prev_recv = data_size;
			cl.do_recv();
		}
		break;
		case IO_SEND:
			ReleaseSendOver(exp_over);   // delete 대신 풀에 반납 (재사용)
			break;
		}
	}
}
