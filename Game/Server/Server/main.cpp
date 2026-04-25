//-----------------------------------------------------------------------------
// 게임서버프로그래밍 강의 서버

#include "pch.h"
#include <array>
#include "protocol.h"
#include "ZombieManager.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "winmm.lib")   // timeBeginPeriod
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// 전역 상태
// ─────────────────────────────────────────────────────────────────────────────
enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

// NavMesh JSON 경로 — 서버 작업 디렉터리(프로젝트 폴더) 기준 상대 경로
static constexpr const char* NAVMESH_PATH     = "../../Client/Resources/NavMesh/NavMesh.json";
static constexpr int         INITIAL_ZOMBIES  = 5;    // 서버 시작 시 스폰할 좀비 수
static constexpr float       TICK_RATE_HZ     = 30.f; // 게임 틱 빈도
static constexpr float       TICK_DT          = 1.f / TICK_RATE_HZ;
static constexpr DWORD       TICK_MS          = static_cast<DWORD>(TICK_DT * 1000.f);

ZombieManager                g_ZombieManager;
unordered_map<int, Vector3>  g_PlayerPositions; // playerId → 월드 위치 (cm)
mutex                        g_Mutex;           // g_PlayerPositions + clients 브로드캐스트 보호
volatile bool                g_bRunning = true;

// ─────────────────────────────────────────────────────────────────────────────
// IOCP 오버랩 구조체
// ─────────────────────────────────────────────────────────────────────────────
class EXP_OVER {
public:
	WSAOVERLAPPED m_over;
	IOType        m_iotype;
	WSABUF        m_wsa;
	char          m_buff[BUF_SIZE];

	EXP_OVER() {
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}
	EXP_OVER(IOType iot) : m_iotype(iot) {
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsa.buf = m_buff;
		m_wsa.len = BUF_SIZE;
	}
};

// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// SESSION
// ─────────────────────────────────────────────────────────────────────────────
class SESSION {
public:
	SESSION() {
		m_is_connected = false;
		m_id           = 999;
		m_client       = INVALID_SOCKET;
		m_recv_over.m_iotype = IO_RECV;
		m_x = 0; m_y = 0;
		m_prev_recv    = 0;
		m_v3Pos        = {};
	}
	~SESSION() {
		if (m_is_connected)
			closesocket(m_client);
	}

	void do_recv() {
		DWORD recv_flag = 0;
		memset(&m_recv_over.m_over, 0, sizeof(m_recv_over.m_over));
		WSARecv(m_client, &m_recv_over.m_wsa, 1, 0, &recv_flag, &m_recv_over.m_over, nullptr);
	}
	void do_send(int num_bytes, char* mess) {
		EXP_OVER* o = new EXP_OVER(IO_SEND);
		o->m_wsa.len = num_bytes;
		memcpy(o->m_buff, mess, num_bytes);
		WSASend(m_client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
	}

	void send_login_success() {
		S2C_LoginResult p;
		p.size    = sizeof(S2C_LoginResult);
		p.type    = S2C_LOGIN_RESULT;
		p.success = true;
		strncpy_s(p.message, "Login successful.", sizeof(p.message));
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_avatar_info() {
		S2C_AvatarInfo p;
		p.size     = sizeof(S2C_AvatarInfo);
		p.type     = S2C_AVATAR_INFO;
		p.playerId = m_id;
		p.x        = m_x;
		p.y        = m_y;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_add_player(int player_id);
	void send_remove_player(int player_id) {
		S2C_RemovePlayer p;
		p.size     = sizeof(S2C_RemovePlayer);
		p.type     = S2C_REMOVE_PLAYER;
		p.playerId = player_id;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_move_packet(int mover);

	// ── 좀비 패킷 ────────────────────────────────────────────────────────────
	void send_spawn_zombie(int nZombieId, const Vector3& v3Pos) {
		S2C_SpawnZombie p;
		p.size     = sizeof(S2C_SpawnZombie);
		p.type     = S2C_SPAWN_ZOMBIE;
		p.zombieId = nZombieId;
		p.x        = v3Pos.x;
		p.y        = v3Pos.y;
		p.z        = v3Pos.z;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_despawn_zombie(int nZombieId) {
		S2C_DespawnZombie p;
		p.size     = sizeof(S2C_DespawnZombie);
		p.type     = S2C_DESPAWN_ZOMBIE;
		p.zombieId = nZombieId;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_zombie_state(int nZombieId, float x, float z, float yaw,
	                       float waypointX, float waypointZ,
	                       ZombieBehaviorState state) {
		S2C_ZombieState p;
		p.size          = sizeof(S2C_ZombieState);
		p.type          = S2C_ZOMBIE_STATE;
		p.zombieId      = nZombieId;
		p.x             = x;
		p.z             = z;
		p.yaw           = yaw;
		p.waypointX     = waypointX;
		p.waypointZ     = waypointZ;
		p.behaviorState = state;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}
	void send_zombie_attack(int nZombieId, int nTargetPlayerId, float fDamage) {
		S2C_ZombieAttack p;
		p.size           = sizeof(S2C_ZombieAttack);
		p.type           = S2C_ZOMBIE_ATTACK;
		p.zombieId       = nZombieId;
		p.targetPlayerId = nTargetPlayerId;
		p.damage         = fDamage;
		do_send(p.size, reinterpret_cast<char*>(&p));
	}

	void process_packet(unsigned char* p);

public:
	SOCKET     m_client;
	EXP_OVER   m_recv_over;
	int        m_id;
	bool       m_is_connected;
	int        m_prev_recv;
	char       m_username[MAX_NAME_LEN];
	short      m_x, m_y;
	Vector3    m_v3Pos; // 수신된 3D 위치 (cm)
};

std::array<SESSION, MAX_PLAYERS> clients;

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
// SESSION 멤버 구현
// ─────────────────────────────────────────────────────────────────────────────
void SESSION::send_add_player(int player_id)
{
	S2C_AddPlayer p;
	p.size     = sizeof(S2C_AddPlayer);
	p.type     = S2C_ADD_PLAYER;
	p.playerId = player_id;
	SESSION& pl = clients[player_id];
	memcpy(p.username, pl.m_username, sizeof(p.username));
	p.x = pl.m_x;
	p.y = pl.m_y;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void SESSION::send_move_packet(int mover)
{
	S2C_MovePlayer p;
	p.size     = sizeof(S2C_MovePlayer);
	p.type     = S2C_MOVE_PLAYER;
	p.playerId = mover;
	p.x        = clients[mover].m_x;
	p.y        = clients[mover].m_y;
	do_send(p.size, reinterpret_cast<char*>(&p));
}

void SESSION::process_packet(unsigned char* p)
{
	PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&p[1]);
	switch (type)
	{
	case C2S_LOGIN:
	{
		C2S_Login* packet = reinterpret_cast<C2S_Login*>(p);
		strncpy_s(m_username, packet->username, MAX_NAME_LEN);
		cout << "Player[" << m_id << "] logged in as " << m_username << endl;
		send_login_success();
		send_avatar_info();

		// 기존 플레이어 정보 교환
		for (auto& other : clients) {
			if (!other.m_is_connected || other.m_id == m_id) continue;
			other.send_add_player(m_id);
			send_add_player(other.m_id);
		}

		// 현재 살아있는 좀비 목록을 새 플레이어에게 전송
		{
			lock_guard<mutex> lock(g_Mutex);
			for (auto& [nId, zombie] : g_ZombieManager.GetZombies()) {
				if (!zombie.bAlive) continue;
				Vector3 v3Pos = zombie.pAgent ? zombie.pAgent->GetPosition() : Vector3{};
				send_spawn_zombie(nId, v3Pos);
			}
		}
	}
	break;

	case C2S_MOVE:
	{
		C2S_Move* packet = reinterpret_cast<C2S_Move*>(p);
		switch (packet->dir) {
		case UP:    m_y++; break;
		case DOWN:  m_y--; break;
		case LEFT:  m_x--; break;
		case RIGHT: m_x++; break;
		}
		for (auto& cl : clients)
			if (cl.m_is_connected)
				cl.send_move_packet(m_id);
	}
	break;

	case C2S_PLAYER_POSITION:
	{
		if (*p < sizeof(C2S_PlayerPosition)) break;
		C2S_PlayerPosition* packet = reinterpret_cast<C2S_PlayerPosition*>(p);
		Vector3 v3Pos(packet->x, packet->y, packet->z);
		m_v3Pos = v3Pos;
		{
			lock_guard<mutex> lock(g_Mutex);
			g_PlayerPositions[m_id] = v3Pos;
		}
	}
	break;

	default:
		cout << "Unknown packet type from player[" << m_id << "].\n";
		break;
	}
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

		// ── AI Tick ──────────────────────────────────────────────────────────
		vector<pair<int,int>> attacks; // (zombieId, targetPlayerId)
		g_ZombieManager.Tick(TICK_DT, playerSnapshots, attacks);

		// ── 결과 브로드캐스트 ────────────────────────────────────────────────
		{
			lock_guard<mutex> lock(g_Mutex);

			// 좀비 상태 전송
			for (auto& [nId, zombie] : g_ZombieManager.GetZombies())
			{
				if (!zombie.bAlive || !zombie.pAgent) continue;

				Vector3 v3Pos     = zombie.pAgent->GetPosition();
				AIBehaviorState   aiState = zombie.pAgent->GetBehaviorState();
				ZombieBehaviorState state =
					static_cast<ZombieBehaviorState>(static_cast<int>(aiState));

				// 다음 waypoint: 현재 경로의 최종 목표
				const auto& dbg   = zombie.pAgent->GetPathDebugInfo();
				float waypointX   = v3Pos.x;
				float waypointZ   = v3Pos.z;
				if (!dbg.Waypoints.empty()) {
					waypointX = dbg.Waypoints.back().x;
					waypointZ = dbg.Waypoints.back().z;
				}

				BroadcastAll([&](SESSION& cl) {
					cl.send_zombie_state(nId, v3Pos.x, v3Pos.z,
					                     zombie.fYaw,
					                     waypointX, waypointZ,
					                     state);
				});
			}

			// 공격 이벤트 전송
			for (auto& [nZombieId, nTargetId] : attacks)
			{
				BroadcastAll([&](SESSION& cl) {
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

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
void send_login_fail(SOCKET client, const char* message)
{
	S2C_LoginResult p;
	p.size    = sizeof(S2C_LoginResult);
	p.type    = S2C_LOGIN_RESULT;
	p.success = false;
	strncpy_s(p.message, message, sizeof(p.message));
	WSABUF wsa_buf{ p.size, reinterpret_cast<char*>(&p) };
	WSASend(client, &wsa_buf, 1, 0, 0, nullptr, nullptr);
}

int main()
{
	// ── ZombieManager 초기화 ─────────────────────────────────────────────────
	if (!g_ZombieManager.Initialize(NAVMESH_PATH))
	{
		cout << "[Server] ZombieManager 초기화 실패. NavMesh 경로 확인: " << NAVMESH_PATH << "\n";
		// NavMesh 없이 계속 진행 (좀비 AI 비활성)
	}
	else
	{
		for (int i = 0; i < INITIAL_ZOMBIES; ++i)
			g_ZombieManager.SpawnZombie();
	}

	// ── WinSock + IOCP 초기화 ────────────────────────────────────────────────
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN server_addr{};
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(server, SOMAXCONN);

	HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	CreateIoCompletionPort((HANDLE)server, h_iocp, -1, 0);

	SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	EXP_OVER accept_over(IO_ACCEPT);
	AcceptEx(server, client_socket, &accept_over.m_buff, 0,
	         sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
	         NULL, &accept_over.m_over);

	// ── 게임 틱 스레드 시작 ──────────────────────────────────────────────────
	HANDLE hTickThread = CreateThread(NULL, 0, GameTickThread, NULL, 0, NULL);
	cout << "[Server] 게임 틱 스레드 시작 (" << TICK_RATE_HZ << "Hz)\n";
	cout << "[Server] 포트 " << PORT << " 에서 대기 중...\n";

	// ── IOCP 이벤트 루프 ─────────────────────────────────────────────────────
	for (int player_index = 0;;)
	{
		DWORD      num_bytes;
		ULONG_PTR  key;
		LPOVERLAPPED over;
		GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);

		if (over == nullptr) {
			error_display(L"GQCS Error: ", WSAGetLastError());
			if (key == -1) exit(-1);
			cout << "client[" << key << "] Disconnected.\n";
			clients[key].m_is_connected = false;
			{
				lock_guard<mutex> lock(g_Mutex);
				g_PlayerPositions.erase(static_cast<int>(key));
			}
			for (auto& cl : clients)
				if (cl.m_is_connected)
					cl.send_remove_player(static_cast<int>(key));
			closesocket(clients[key].m_client);
			clients[key].m_client = INVALID_SOCKET;
			continue;
		}

		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(over);

		switch (exp_over->m_iotype)
		{
		case IO_ACCEPT:
		{
			cout << "Client connected.\n";
			player_index = -1;
			for (int i = 0; i < MAX_PLAYERS; ++i)
				if (!clients[i].m_is_connected) { player_index = i; break; }

			if (-1 == player_index) {
				send_login_fail(client_socket, "Server is full.");
				closesocket(client_socket);
			}
			else {
				CreateIoCompletionPort((HANDLE)client_socket, h_iocp, player_index, 0);
				clients[player_index].m_is_connected = true;
				clients[player_index].m_client       = client_socket;
				clients[player_index].m_x            = 0;
				clients[player_index].m_y            = 0;
				clients[player_index].m_id           = player_index;
				clients[player_index].m_prev_recv    = 0;
				clients[player_index].do_recv();
			}

			client_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			AcceptEx(server, client_socket, &accept_over.m_buff, 0,
			         sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
			         NULL, &accept_over.m_over);
		}
		break;

		case IO_RECV:
		{
			int     nIdx = static_cast<int>(key);
			SESSION& cl  = clients[nIdx];
			if (num_bytes == 0) {
				cl.m_is_connected = false;
				for (auto& other : clients)
					if (other.m_is_connected)
						other.send_remove_player(nIdx);
				closesocket(cl.m_client);
				cl.m_client = INVALID_SOCKET;
				{
					lock_guard<mutex> lock(g_Mutex);
					g_PlayerPositions.erase(nIdx);
				}
				break;
			}

			unsigned char* p = reinterpret_cast<unsigned char*>(exp_over->m_buff);
			int data_size = num_bytes + cl.m_prev_recv;
			while (data_size > 0) {
				int packet_size = p[0];
				if (packet_size == 0 || packet_size > data_size) break;
				cl.process_packet(p);
				p         += packet_size;
				data_size -= packet_size;
			}
			if (data_size > 0) {
				memmove(cl.m_recv_over.m_buff, p, data_size);
				cl.m_prev_recv = data_size;
			}
			cl.do_recv();
		}
		break;

		case IO_SEND:
		{
			EXP_OVER* o = reinterpret_cast<EXP_OVER*>(over);
			delete o;
		}
		break;

		default:
			cout << "Unknown IO type.\n";
			exit(-1);
		}
	}

	g_bRunning = false;
	WaitForSingleObject(hTickThread, 3000);
	CloseHandle(hTickThread);
	closesocket(server);
	WSACleanup();
}
