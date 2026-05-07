#include "pch.h"

#include "ZombieManager.h"
#include "Session.h"
#include "Room.h"

#include "protocol.h"

using namespace std;

// NavMesh JSON 경로 — 서버 작업 디렉터리(프로젝트 폴더) 기준 상대 경로
static constexpr const char* NAVMESH_PATH = "../../Client/Resources/NavMesh/NavMesh.json";
static constexpr int         INITIAL_ZOMBIES = 5;    // 서버 시작 시 스폰할 좀비 수
static constexpr float       TICK_RATE_HZ = 30.f; // 게임 틱 빈도
static constexpr float       TICK_DT = 1.f / TICK_RATE_HZ;
static constexpr DWORD       TICK_MS = static_cast<DWORD>(TICK_DT * 1000.f);

ZombieManager                g_ZombieManager;
unordered_map<int, Vector3>  g_PlayerPositions; // playerId → 월드 위치 (cm)
mutex                        g_Mutex;           // g_PlayerPositions + clients 브로드캐스트 보호
volatile bool                g_bRunning = true;

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
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::wcout << msg << L" === 에러 " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}
void send_login_fail(SOCKET client, const char* message)
{
	S2C_LoginResult p;
	p.size = sizeof(S2C_LoginResult);
	p.type = S2C_LOGIN_RESULT;
	p.success = false;
	strncpy_s(p.message, message, sizeof(p.message));
	WSABUF wsa_buf{ p.size, reinterpret_cast<char*>(&p) };
	WSASend(client, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// 전체 클라이언트 브로드캐스트 헬퍼 (게임 틱 스레드에서도 호출)
// ─────────────────────────────────────────────────────────────────────────────
template<typename Fn>
static void BroadcastAll(Fn fn)
{
	for (auto& cl : clients)
		if (cl.m_is_connected)
			fn(cl);
}

// ─────────────────────────────────────────────────────────────────────────────
// 게임 틱 스레드 (30Hz) — AI 업데이트 + 좀비 상태 브로드캐스트
// ─────────────────────────────────────────────────────────────────────────────
static DWORD WINAPI GameTickThread(LPVOID)
{
	timeBeginPeriod(1); // 1ms 정밀도 타이머 활성화

	while (g_bRunning)
	{
		DWORD dwTickStart = timeGetTime();

		// ── 플레이어 위치 스냅샷 ─────────────────────────────────────────────
		unordered_map<int, Vector3> playerSnapshots;
		{
			lock_guard<mutex> lock(g_Mutex);
			playerSnapshots = g_PlayerPositions;
		}

		// ── AI Tick (락 밖에서 실행 — AIManager 자체 스레드 안전) ───────────
		vector<pair<int, int>> attacks; // (zombieId, targetPlayerId)
		g_ZombieManager.Tick(TICK_DT, playerSnapshots, attacks);

		// ── 결과 브로드캐스트 ────────────────────────────────────────────────
		{
			lock_guard<mutex> lock(g_Mutex);

			// 좀비 상태 전송
			for (auto& [nId, zombie] : g_ZombieManager.GetZombies())
			{
				if (!zombie.bAlive || !zombie.pAgent) continue;

				Vector3 v3Pos = zombie.pAgent->GetPosition();
				ZombieBehaviorState state = static_cast<ZombieBehaviorState>(
					static_cast<int>(zombie.pAgent->GetBehaviorState()));

				// 다음 waypoint: 경로의 마지막 웨이포인트 (없으면 현재 위치)
				const auto& dbg = zombie.pAgent->GetPathDebugInfo();
				float waypointX = v3Pos.x;
				float waypointZ = v3Pos.z;
				if (!dbg.Waypoints.empty()) {
					waypointX = dbg.Waypoints.back().x;
					waypointZ = dbg.Waypoints.back().z;
				}

				BroadcastAll([&](Session& cl) {
					cl.send_zombie_state(nId, v3Pos.x, v3Pos.z,
						zombie.fYaw, waypointX, waypointZ, state);
				});
			}

			// 공격 이벤트 전송
			for (auto& [nZombieId, nTargetId] : attacks)
			{
				BroadcastAll([&](Session& cl) {
					cl.send_zombie_attack(nZombieId, nTargetId, 10.f);
				});
			}
		}

		// ── 다음 틱까지 대기 ─────────────────────────────────────────────────
		DWORD dwElapsed = timeGetTime() - dwTickStart;
		if (dwElapsed < TICK_MS)
			Sleep(TICK_MS - dwElapsed);
	}

	timeEndPeriod(1);
	return 0;
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
	// ── ZombieManager 초기화 + 초기 좀비 스폰 ───────────────────────────────
	if (!g_ZombieManager.Initialize(NAVMESH_PATH))
	{
		std::cout << "[Server] ZombieManager 초기화 실패. NavMesh 경로 확인: " << NAVMESH_PATH << "\n";
	}
	else
	{
		for (int i = 0; i < INITIAL_ZOMBIES; ++i)
			g_ZombieManager.SpawnZombie();
	}

	// ── 게임 틱 스레드 시작 (30Hz) ───────────────────────────────────────────
	HANDLE hTickThread = CreateThread(NULL, 0, GameTickThread, NULL, 0, NULL);

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
