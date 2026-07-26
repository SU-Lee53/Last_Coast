#include "pch.h"
#include "GameWorld.h"

bool GameWorld::Initialize(const std::string& navMeshPath, const std::string& spawnPointPath, const std::string& attackAnimPath,
	const std::string& sceneJsonPath, const std::string& modelDirectory, const std::string& checkpointPath)
{
	bool bOk = true;

	// ── ZombieManager 초기화 ────────────────────────────────────────────────
	if (!m_ZombieManager.Initialize(navMeshPath))
	{
		// NavMesh 로드 실패 = 좀비가 길을 못 찾음(추격 불가). 조용히 넘어가지 말고 반드시 표시.
		std::cout << "[Server][ERROR] ZombieManager/NavMesh 초기화 실패! 경로 확인: " << navMeshPath << "\n";
		bOk = false;
	}
	else
	{
		std::cout << "[Server] NavMesh 로드 성공: " << navMeshPath << "\n";
		// 별도 파일에서 좀비 스폰 포인트 로드 (언리얼 SaveSpawnPointsToJson 출력)
		// 실제 스폰은 게임 틱의 드립 스포너가 랜덤 포인트에서 최대치까지 채운다
		m_ZombieManager.LoadSpawnPoints(spawnPointPath);
	}

	// ── 공격 애니메이션 길이 로드 (2종 — 인덱스 1은 "<이름>1.bin" 규칙: Zombie Attack1.bin) ──
	m_ZombieManager.LoadAttackAnimDuration(0, attackAnimPath);
	std::string attackAnim1Path = attackAnimPath;
	attackAnim1Path.insert(attackAnim1Path.size() - 4, "1"); // ".bin" 앞에 "1" 삽입
	m_ZombieManager.LoadAttackAnimDuration(1, attackAnim1Path);

	// ── 정적 OBB 공간 분할 초기화 (사격 차폐 판정용) ────────────────────────
	if (!m_SpatialGrid.LoadFromSceneFile(sceneJsonPath, modelDirectory))
	{
		//std::cout << "[Server] SpatialGrid 초기화 실패. 씬/모델 경로 확인.\n";
	}

	// ── 체크포인트 로드 + 이벤트 배정 (언리얼 SaveCheckpointsToJson 출력) ─────
	m_Checkpoints.Load(checkpointPath);
	m_Checkpoints.AssignEvents();

	return bOk;
}
