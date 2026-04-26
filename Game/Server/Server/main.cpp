#include "pch.h"
#include "Session.h"
#include "Room.h"

std::array<Session, MAX_PLAYERS> clients;
std::array<Room, MAX_ROOMS> rooms;

SOCKET g_server;
HANDLE g_iocp;

Room* find_empty_room()
{
	for (auto& room : rooms) {
		if (!room.is_full())
			return &room;
	}
	return nullptr;
}

void error_display(const wchar_t* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg;
	std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);   // 디버깅 용
	LocalFree(lpMsgBuf);
}

void send_login_fail(SOCKET client, const char* message)
{
	S2C_LoginResult packet;
	packet.size = sizeof(S2C_LoginResult);
	packet.type = S2C_LOGIN_RESULT;
	packet.success = false;
	strncpy_s(packet.message, message, sizeof(packet.message));
	WSABUF wsa_buf;
	wsa_buf.buf = reinterpret_cast<char*>(&packet);
	wsa_buf.len = packet.size;
	WSASend(client, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

void disconnect(int id)
{
	Session& cl = clients[id];

	if (!cl.m_is_connected) return;

	std::cout << "Client[" << id << "] disconnected.\n";

	// Room에서 제거
	if (cl.m_room != nullptr) {
		Room* room = cl.m_room;

		for (int other_id : room->players) {
			if (other_id == -1 || other_id == id) continue;
			clients[other_id].send_remove_player(id);
		}

		room->remove_player(id);
		cl.m_room = nullptr;
	}

	closesocket(cl.m_client);
	cl.m_client = INVALID_SOCKET;
	cl.m_is_connected = false;
}

void worker_thread()
{
	while (true) {
		DWORD num_bytes = 0;
		ULONG_PTR long_key = 0;
		LPOVERLAPPED over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_iocp, &num_bytes, &long_key, &over, INFINITE);
		int key = static_cast<int>(long_key);
		if (FALSE == ret) {
			error_display(L"GQCS Error", WSAGetLastError());
			continue;
		}
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

		switch (exp_over->m_iotype) {
		case IO_ACCEPT:
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
				send_login_fail(exp_over->m_client_socket, "Server Full");
				closesocket(exp_over->m_client_socket);
			}
			// 접속 허용
			else {
				Room* room = find_empty_room();

				if (room == nullptr) {
					send_login_fail(exp_over->m_client_socket, "No Room Available");
					closesocket(exp_over->m_client_socket);
				}
				else {
					room->add_player(player_index);
					CreateIoCompletionPort((HANDLE)exp_over->m_client_socket, g_iocp, player_index, 0);

					clients[player_index].init(exp_over->m_client_socket, player_index, room);
					clients[player_index].send_login_success();

					Room* room = clients[player_index].m_room;

					for (int other_id : room->players) {
						if (other_id == -1 || other_id == player_index)
							continue;

						clients[other_id].send_add_player(player_index);
						clients[player_index].send_add_player(other_id);
					}

					clients[player_index].do_recv();
					std::cout << "Client[" << player_index << "] Connected. " << "Room assigned.\n";
				}
			}

			// 다음 Accept 준비
			exp_over->m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&exp_over->m_over, sizeof(exp_over->m_over));
			AcceptEx(g_server, exp_over->m_client_socket, exp_over->m_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &exp_over->m_over);
		}
		break;

		case IO_RECV:
		{
			if (0 == num_bytes) {
				disconnect(key);
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
					disconnect(key);
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
		case IO_SEND: {
			// cout << "Message sent. to client[" << key << "]\n";
			EXP_OVER* o = reinterpret_cast<EXP_OVER*>(over);
			delete o;
		}
					break;
		}
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_server, SOMAXCONN);
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort((HANDLE)g_server, g_iocp, -1, 0);

	EXP_OVER accept_over(IO_ACCEPT);
	accept_over.m_client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	AcceptEx(g_server, accept_over.m_client_socket, &accept_over.m_buff, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, &accept_over.m_over);

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread);
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_server);
	WSACleanup();
}
