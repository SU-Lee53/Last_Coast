#include "pch.h"

#include "GameWorld.h"
#include "GameLoop.h"
#include "Network.h"
#include "DBManager.h"

// 리소스 경로 — 서버 작업 디렉터리(프로젝트 폴더) 기준 상대 경로
static const std::string NAVMESH_PATH         = "../../Client/Resources/NavMesh/GAME.json";
static const std::string SCENE_JSON_PATH      = "../../Client/Resources/Scenes/GAME.json";
static const std::string SPAWN_JSON_PATH      = "../../Client/Resources/Scenes/GAME_SpawnPoints.json";
static const std::string MODEL_DIRECTORY      = "../../Client/Resources/Models";
static const std::string ATTACK_ANIM_PATH     = "../../Client/Resources/Animations/Zombie Attack.bin";
static const std::string CHECKPOINT_JSON_PATH = "../../Client/Resources/Scenes/GAME_Checkpoints.json"; // 언리얼 SaveCheckpointsToJson 출력

int main()
{
	// ── 공유 자원 초기화 (정적 OBB 로드 + 방별 월드 생성용 리소스 경로 보관) ──
	// NavMesh/스폰포인트/체크포인트는 방마다 게임 시작 시 GameWorld가 개별 로드한다.
	SHARED->Initialize(NAVMESH_PATH, SPAWN_JSON_PATH, ATTACK_ANIM_PATH,
		SCENE_JSON_PATH, MODEL_DIRECTORY, CHECKPOINT_JSON_PATH);

	// ── DBManager 초기화 ────────────────────────────────────────────────────
	if (!DB->Initialize(L"LastCoastDB"))
	{
		std::cout << "[Server] DBManager 초기화 실패. ODBC DSN을 확인하세요.\n";
	}

	// ── 게임 틱 스레드 시작 (30Hz) ───────────────────────────────────────────
	std::thread tickThread([] { GAMELOOP->Run(); });

	// ── IOCP 네트워크 시작 + 워커 스레드 ────────────────────────────────────
	NETWORK->Init(PORT);

	std::vector<std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back([] { NETWORK->WorkerLoop(); });
	for (auto& th : worker_threads)
		th.join();

	SHARED->Stop();
	tickThread.join();
	NETWORK->Shutdown();
}
