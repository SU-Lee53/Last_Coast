#include "pch.h"
#include "GameWorld.h"
#include "Room.h"

// ─────────────────────────────────────────────────────────────────────────────
// ServerShared
// ─────────────────────────────────────────────────────────────────────────────

void ServerShared::Initialize(const std::string& navMeshPath, const std::string& spawnPointPath,
	const std::string& attackAnimPath, const std::string& sceneJsonPath,
	const std::string& modelDirectory, const std::string& checkpointPath)
{
	// 방별 GameWorld 생성 시 재사용할 경로 보관
	m_navMeshPath    = navMeshPath;
	m_spawnPointPath = spawnPointPath;
	m_attackAnimPath = attackAnimPath;
	m_checkpointPath = checkpointPath;

	// ── 정적 OBB 공간 분할 초기화 (사격 차폐 판정용) — 읽기 전용이라 전 방 공유 ──
	if (!m_SpatialGrid.LoadFromSceneFile(sceneJsonPath, modelDirectory))
	{
		//std::cout << "[Server] SpatialGrid 초기화 실패. 씬/모델 경로 확인.\n";
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// GameWorld (방별 인스턴스)
// ─────────────────────────────────────────────────────────────────────────────

GameWorld::GameWorld(Room& room)
	: m_Room(room)
	, m_Combat(m_ZombieManager, SHARED->GetGrid())
{
}

bool GameWorld::Initialize()
{
	bool bOk = true;

	// ── ZombieManager 초기화 (방 전용 IAIManager + NavMesh 로드) ────────────
	if (!m_ZombieManager.Initialize(SHARED->NavMeshPath()))
	{
		// NavMesh 로드 실패 = 좀비가 길을 못 찾음(추격 불가). 조용히 넘어가지 말고 반드시 표시.
		std::cout << "[Server][ERROR] ZombieManager/NavMesh 초기화 실패! 경로 확인: " << SHARED->NavMeshPath() << "\n";
		bOk = false;
	}
	else
	{
		std::cout << "[Server] Room[" << m_Room.room_id << "] NavMesh 로드 성공: " << SHARED->NavMeshPath() << "\n";
		// 별도 파일에서 좀비 스폰 포인트 로드 (언리얼 SaveSpawnPointsToJson 출력)
		// 실제 스폰은 게임 틱의 드립 스포너가 랜덤 포인트에서 최대치까지 채운다
		m_ZombieManager.LoadSpawnPoints(SHARED->SpawnPointPath());
	}

	// ── 공격 애니메이션 길이 로드 (2종 — 인덱스 1은 "<이름>1.bin" 규칙: Zombie Attack1.bin) ──
	m_ZombieManager.LoadAttackAnimDuration(0, SHARED->AttackAnimPath());
	std::string attackAnim1Path = SHARED->AttackAnimPath();
	attackAnim1Path.insert(attackAnim1Path.size() - 4, "1"); // ".bin" 앞에 "1" 삽입
	m_ZombieManager.LoadAttackAnimDuration(1, attackAnim1Path);

	// ── 체크포인트 로드 + 이벤트 배정 (언리얼 SaveCheckpointsToJson 출력) ─────
	m_Checkpoints.Load(SHARED->CheckpointPath());
	m_Checkpoints.AssignEvents();

	return bOk;
}
