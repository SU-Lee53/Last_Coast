#include "pch.h"

#include "ZombieManager.h"
#include "ServerSpatialGrid.h"
#include "Session.h"
#include "Room.h"

#include "protocol.h"

#define JSON_HAS_RANGES 0
#include <Includes/nlohmann_json/json.hpp>

using namespace std;

// NavMesh JSON 경로 — 서버 작업 디렉터리(프로젝트 폴더) 기준 상대 경로
static constexpr const char* NAVMESH_PATH      = "../../Client/Resources/NavMesh/DEMO.json";
static constexpr const char* SCENE_JSON_PATH   = "../../Client/Resources/Scenes/DEMO.json";
static constexpr const char* SPAWN_JSON_PATH   = "../../Client/Resources/Scenes/DEMO_SpawnPoints.json";
static constexpr const char* MODEL_DIRECTORY   = "../../Client/Resources/Models";
static constexpr const char* ATTACK_ANIM_PATH  = "../../Client/Resources/Animations/Zombie Attack.bin";
static constexpr int         INITIAL_ZOMBIES = 100;    // 서버 시작 시 스폰할 좀비 수
static constexpr float       TICK_RATE_HZ = 30.f; // 게임 틱 빈도
static constexpr float       TICK_DT = 1.f / TICK_RATE_HZ;
static constexpr Vector3 m_v3SpawnPosition = { 50600.f, -3590.f, 22000.f }; // NavMesh → 월드 좌표 오프셋 (cm)
static constexpr DWORD       TICK_MS = static_cast<DWORD>(TICK_DT * 1000.f);
static constexpr DWORD       ZOMBIE_SEND_INTERVAL_MS = 100; // 좀비 상태 정기 전송 간격 (0.2초)
static constexpr DWORD       ZOMBIE_SPAWN_INTERVAL_MS = 4000; // 좀비 스폰 간격
static constexpr const char* CHECKPOINT_JSON_PATH = "../../Client/Resources/Scenes/DEMO_Checkpoints.json"; // 언리얼 SaveCheckpointsToJson 출력

ZombieManager                g_ZombieManager;
ServerSpatialGrid            g_SpatialGrid;     // 정적 OBB 공간 분할 (사격 차폐 판정)
concurrency::concurrent_unordered_map<int, Vector3>  g_PlayerPositions; // playerId → 월드 위치 (cm) (lock-free)
volatile bool                g_bRunning = true;

std::array<Session, MAX_PLAYERS> clients;
std::array<Room, MAX_ROOMS> rooms;

SOCKET g_server;
HANDLE g_iocp;

// ── 체크포인트(전원 도달 트리거) ────────────────────────────────────────────────
// 맵이 일직선(+X 전진)이라 진행도 = pos.x. 모든 연결 플레이어의 min(x)가 체크포인트 x를
// 넘으면(= 가장 뒤처진 플레이어까지 도달) 1회 events 를 브로드캐스트.
struct EventEmit {
	int     eventId;          // GameEventId
	Vector3 pos;              // 위치 의존 이벤트용(폭발 등). 무관하면 {}
	float   fTargetValue;
	float   fDuration;
	int     presetId;         // EnvironmentPresetId (GE_ENVIRONMENT 전용)
	float   fFreezeDuration;  // 컷씬 길이(초). >0 이면 그 동안 서버 좀비 AI 정지(온라인 스냅 방지). 비컷씬=0

	// 명시적 생성자 — 중첩 중괄호 {{...}}로 vector<EventEmit> 초기화 시 MSVC 변환 오류 방지.
	EventEmit(int eventId_, const Vector3& pos_, float fTargetValue_,
	          float fDuration_, int presetId_, float fFreezeDuration_)
		: eventId(eventId_), pos(pos_), fTargetValue(fTargetValue_)
		, fDuration(fDuration_), presetId(presetId_), fFreezeDuration(fFreezeDuration_) {}
};
struct Checkpoint {
	Vector3                v3Pos;          // 익스포트 위치(cm). x 좌표를 임계값으로 사용
	std::vector<EventEmit> events;         // 도달 시 발사할 이벤트들
	bool                   bFired = false; // 1회성
};
std::vector<Checkpoint> g_Checkpoints;

// 이 시각(timeGetTime, ms)까지 좀비 AI/이동/공격/스폰 정지 — 클라 컷씬과 동기화해 종료 후 스냅 방지.
DWORD g_dwZombieFreezeUntil = 0;

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
	EXP_OVER* o = new EXP_OVER(IO_SEND);
	o->m_wsa.len = p.size;
	memcpy(o->m_buff, &p, p.size);
	WSASend(client, &o->m_wsa, 1, 0, 0, &o->m_over, nullptr);
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
// 체크포인트 로드 + 이벤트 배정
// ─────────────────────────────────────────────────────────────────────────────

// DEMO_Checkpoints.json("Checkpoints") 에서 체크포인트 위치 로드 (익스포터가 이름 끝 숫자순 정렬).
static int LoadCheckpoints(const char* path)
{
	g_Checkpoints.clear();

	std::ifstream in(path);
	if (!in) {
		std::cout << "[Checkpoint] file not found: " << path << "\n";
		return 0;
	}

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("Checkpoints"))
		return 0;

	for (const auto& jc : j["Checkpoints"]) {
		const auto& wm = jc["Transform"]["WorldMatrix"]; // 행 우선 4x4 — translation = 12,13,14
		Vector3 v3Pos(wm[12].get<float>(), wm[13].get<float>(), wm[14].get<float>());
		g_Checkpoints.push_back(Checkpoint{ v3Pos, {}, false });
	}

	std::cout << "[Checkpoint] loaded: " << g_Checkpoints.size() << "\n";
	return static_cast<int>(g_Checkpoints.size());
}

// 체크포인트 index →
// (위치는 익스포트 순서 = Checkpoint_1, Checkpoint_2 … 순)
static void AssignCheckpointEvents()
{
	auto Set = [](size_t i, std::vector<EventEmit> ev) {
		if (i < g_Checkpoints.size()) g_Checkpoints[i].events = std::move(ev);
	};

	// 예시 배정 — 한 체크포인트가 여러 이벤트를 동시에 쏠 수 있음.
	//Set(0, { { GE_HELICOPTER_CRASH, {}, 0.f, 0.0f, 0, 5.0f } });
	Set(0, { { GE_ENVIRONMENT, {}, 0.f, 8.0f, EP_SUNSET, 8.0f } });   // 0번 도착 → 석양 (정지 8초)
	//Set(1, { { GE_ENVIRONMENT, {}, 0.f, 3.0f, EP_NIGHT,  0.f } });   // 1번 도착 → 야간 (정지 안함)
	//Set(2, { { GE_HELICOPTER_CRASH, {}, 0.f, 0.f, 0,     5.0f },     // 2번 도착 → 헬기추락 (정지 5초)
	//		 { GE_ENVIRONMENT, {}, 0.f, 2.0f, EP_HORROR, 0.f } });   //            + 호러룩 (정지 안함)
}

// ─────────────────────────────────────────────────────────────────────────────
// 게임 틱 스레드 (30Hz) — AI 업데이트 + 좀비 상태 브로드캐스트
// ─────────────────────────────────────────────────────────────────────────────
static DWORD WINAPI GameTickThread(LPVOID)
{
	timeBeginPeriod(1); // 1ms 정밀도 타이머 활성화
	DWORD dwPrevTickTime = timeGetTime();
	DWORD dwLastSpawnTime = dwPrevTickTime;

	while (g_bRunning)
	{
		DWORD dwTickStart = timeGetTime();

		// 실제 경과 시간 측정 (서버-클라이언트 시간 동기화)
		float fActualDT = (dwTickStart - dwPrevTickTime) * 0.001f;
		fActualDT = std::clamp(fActualDT, 0.001f, 0.1f); // 1ms~100ms 범위 제한
		dwPrevTickTime = dwTickStart;

		// 컷씬 중(온라인): 좀비 AI/이동/공격/스폰 정지. 클라도 정지 상태라 종료 후 스냅 없음.
		const bool bZombieFrozen = (dwTickStart < g_dwZombieFreezeUntil);

		// ── 플레이어 위치 스냅샷 (clients 배열에서 직접 읽기) ────────────────
		unordered_map<int, Vector3> playerSnapshots;
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (clients[i].m_is_connected) {
				float x = clients[i].m_transform.m[3][0];
				float y = clients[i].m_transform.m[3][1];
				float z = clients[i].m_transform.m[3][2];
				playerSnapshots[i] = Vector3{ x, y, z };
			}
		}

		// ── 좀비 AI/이동/공격/스폰 (컷씬 정지 중에는 전체 스킵) ────────────────────
		if (!bZombieFrozen)
		{
		// ── AI Tick ──────────────────────────────────────────────────────────
		vector<pair<int, int>> attacks;     // 데미지 발동 (Notify 딜레이 후)
		vector<pair<int, int>> attackAnims; // 공격 모션 시작 (즉시)
		g_ZombieManager.Tick(fActualDT, playerSnapshots, attacks, attackAnims);

		// 좀비 상태 전송 (상태 변화 + 위치 변화 시 즉시, 아니면 100ms 간격)
		for (auto& [nId, zombie] : g_ZombieManager.GetZombies())
		{
			if (!zombie.bAlive || !zombie.pAgent) continue;

			int nCurState = static_cast<int>(zombie.pAgent->GetBehaviorState());
			Vector3 v3Pos = zombie.pAgent->GetPosition();

			bool bStateChanged = (nCurState != zombie.nLastSentState);
			bool bIntervalElapsed = (dwTickStart - zombie.dwLastSendTime >= ZOMBIE_SEND_INTERVAL_MS);

			if (!bStateChanged && !bIntervalElapsed) continue;

			ZombieBehaviorState state = static_cast<ZombieBehaviorState>(nCurState);

			const auto& dbg = zombie.pAgent->GetPathDebugInfo();
			float waypointX = v3Pos.x;
			float waypointZ = v3Pos.z;
			//CheckList 이거 수정해야함 문제점이 많음.
			if (!dbg.Waypoints.empty()) {
				waypointX = dbg.Waypoints.back().x;
				waypointZ = dbg.Waypoints.back().z;
			}

			BroadcastAll([&](Session& cl) {
				cl.send_zombie_state(nId, v3Pos.x, v3Pos.z,
					zombie.fYaw, waypointX, waypointZ, state);
			});

			zombie.dwLastSendTime = dwTickStart;
			zombie.nLastSentState = nCurState;
		}

		bool bSpawnElapsed = (dwTickStart - dwLastSpawnTime >= ZOMBIE_SPAWN_INTERVAL_MS);

		if(bSpawnElapsed){
			if(g_ZombieManager.GetZombies().size() >= INITIAL_ZOMBIES) {
				//std::cout << "[Server] 최대 좀비 수 도달. 추가 스폰 생략.\n";
				dwLastSpawnTime = dwTickStart;
			}
			else {
				// 스폰 포인트 중 랜덤 하나 선택 (없으면 랜덤 NavMesh로 폴백)
				int nZombieId = g_ZombieManager.SpawnZombie(g_ZombieManager.GetRandomSpawnPoint());
				dwLastSpawnTime = dwTickStart;
				auto& zombie = g_ZombieManager.GetZombies()[nZombieId];
				BroadcastAll([&](Session& cl) {
					cl.send_spawn_zombie(nZombieId, zombie.pAgent->GetPosition());
					});
			}
		}

		// 공격 모션 시작 (즉시 — 몽타주 재생용, 데미지 0)
		for (auto& [nZombieId, nTargetId] : attackAnims)
		{
			BroadcastAll([&](Session& cl) {
				cl.send_zombie_attack(nZombieId, nTargetId, 0.f);
			});
		}

		// 공격 데미지 발동 (Notify 딜레이 후 — 실제 데미지)
		for (auto& [nZombieId, nTargetId] : attacks)
		{
			BroadcastAll([&](Session& cl) {
				cl.send_zombie_attack(nZombieId, nTargetId, 10.f);
			});
		}
		} // if (!bZombieFrozen)

		// ── 체크포인트 트리거: 전원이 지점 도달 시 이벤트 발사 ────────────────────
		// 진행축 +X. 모든 연결 플레이어의 min(x)가 체크포인트 x를 넘으면 1회 발사.
		if (!playerSnapshots.empty() && !g_Checkpoints.empty())
		{
			float fMinX = playerSnapshots.begin()->second.x;
			for (const auto& [nId, v3Pos] : playerSnapshots)
				fMinX = std::min(fMinX, v3Pos.x);

			for (auto& cp : g_Checkpoints)
			{
				if (cp.bFired || fMinX < cp.v3Pos.x) continue;
				cp.bFired = true;

				float fMaxFreeze = 0.f;
				for (const auto& e : cp.events) {
					BroadcastAll([&](Session& cl) {
						cl.send_game_event(e.eventId, e.pos, e.fTargetValue, e.fDuration, e.presetId);
					});
					fMaxFreeze = std::max(fMaxFreeze, e.fFreezeDuration);
				}

				// 컷씬이면 그 길이(+0.5초 버퍼)만큼 서버 좀비 정지 — 클라 컷씬 종료 시점까지 좀비 고정.
				if (fMaxFreeze > 0.f) {
					DWORD dwUntil = dwTickStart + static_cast<DWORD>((fMaxFreeze + 0.5f) * 1000.f);
					g_dwZombieFreezeUntil = std::max(g_dwZombieFreezeUntil, dwUntil);
				}

				std::cout << "[Checkpoint] fired at x=" << cp.v3Pos.x
					<< " (" << cp.events.size() << " events, freeze " << fMaxFreeze << "s)\n";
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

	//std::cout << "Client[" << id << "] disconnected.\n";

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

					// Nagle 비활성화 — 작은 패킷 즉시 전송 (좀비 상태 패킷 지연 방지)
					BOOL bNoDelay = TRUE;
					setsockopt(exp_over->m_client_socket, IPPROTO_TCP, TCP_NODELAY,
					           reinterpret_cast<const char*>(&bNoDelay), sizeof(bNoDelay));

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
					//std::cout << "Client[" << player_index << "] Connected. " << "Room assigned.\n";
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
		//std::cout << "[Server] ZombieManager 초기화 실패. NavMesh 경로 확인: " << NAVMESH_PATH << "\n";
	}
	else
	{
		// 별도 파일에서 좀비 스폰 포인트 로드 (언리얼 SaveSpawnPointsToJson 출력)
		// 실제 스폰은 게임 틱의 드립 스포너가 랜덤 포인트에서 INITIAL_ZOMBIES까지 채운다
		g_ZombieManager.LoadSpawnPoints(SPAWN_JSON_PATH);
	}

	// ── 체크포인트 로드 + 이벤트 배정 (언리얼 SaveCheckpointsToJson 출력) ─────────
	LoadCheckpoints(CHECKPOINT_JSON_PATH);
	AssignCheckpointEvents();

	// ── 공격 애니메이션 길이 로드 ────────────────────────────────────────────
	g_ZombieManager.LoadAttackAnimDuration(ATTACK_ANIM_PATH);

	// ── 정적 OBB 공간 분할 초기화 (사격 차폐 판정용) ────────────────────────
	if (!g_SpatialGrid.LoadFromSceneFile(SCENE_JSON_PATH, MODEL_DIRECTORY))
	{
		//std::cout << "[Server] SpatialGrid 초기화 실패. 씬/모델 경로 확인.\n";
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
	::bind(g_server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
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
