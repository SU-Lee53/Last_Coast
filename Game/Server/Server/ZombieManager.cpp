#include "pch.h"
#include "ZombieManager.h"

bool ZombieManager::Initialize(const std::string& strNavMeshPath)
{
	m_pAIManager = CreateAIManager();
	if (!m_pAIManager)
		return false;

	if (!m_pAIManager->LoadNavMesh(strNavMeshPath))
	{
		std::cout << "[ZombieManager] NavMesh 로드 실패: " << strNavMeshPath << "\n";
		return false;
	}

	std::cout << "[ZombieManager] NavMesh 로드 완료: " << strNavMeshPath << "\n";
	return true;
}

int ZombieManager::SpawnZombie()
{
	if (!m_pAIManager)
		return -1;

	auto pNavMesh = m_pAIManager->GetNavMesh();
	if (!pNavMesh)
		return -1;

	Vector3 v3SpawnPos = pNavMesh->GetRandomPoint();

	auto pAgent = m_pAIManager->CreateAgent();
	if (!pAgent)
		return -1;

	pAgent->SetPosition(v3SpawnPos);
	pAgent->SetMoveSpeed(110.f); // 클라이언트와 동일 (1.1m/s)

	int nId = m_nNextId++;
	ServerZombie& zombie  = m_Zombies[nId];
	zombie.nId            = nId;
	zombie.pAgent         = pAgent;
	zombie.fHP            = 100.f;
	zombie.bAlive         = true;
	zombie.v3PrevPos      = v3SpawnPos;
	zombie.fYaw           = 0.f;

	std::cout << "[ZombieManager] 좀비 스폰 id=" << nId
	          << " pos=(" << v3SpawnPos.x << "," << v3SpawnPos.z << ")\n";
	return nId;
}

void ZombieManager::DespawnZombie(int nId)
{
	auto it = m_Zombies.find(nId);
	if (it == m_Zombies.end()) return;

	m_Zombies.erase(it);
	std::cout << "[ZombieManager] 좀비 디스폰 id=" << nId << "\n";
}

void ZombieManager::Tick(float fDeltaTime,
                         const std::unordered_map<int, Vector3>& playerPositions,
                         std::vector<std::pair<int,int>>& outAttacks)
{
	if (!m_pAIManager) return;

	for (auto& [nId, zombie] : m_Zombies)
	{
		if (!zombie.bAlive || !zombie.pAgent) continue;

		Vector3 v3ZombiePos = zombie.pAgent->GetPosition();

		// 가장 가까운 플레이어를 타겟으로 선택
		int     nTargetId   = -1;
		float   fMinDistSq  = FLT_MAX;
		Vector3 v3TargetPos = {};

		for (auto& [nPlayerId, v3PlayerPos] : playerPositions)
		{
			float fDistSq = Vector3::DistanceSquared(v3ZombiePos, v3PlayerPos);
			if (fDistSq < fMinDistSq)
			{
				fMinDistSq  = fDistSq;
				nTargetId   = nPlayerId;
				v3TargetPos = v3PlayerPos;
			}
		}

		if (nTargetId >= 0)
		{
			float fDist    = std::sqrtf(fMinDistSq);
			bool  bVisible = IsVisible(v3ZombiePos, v3TargetPos, m_fSightRange);
			bool  bHeard   = (fDist <= m_fHearingRange);

			zombie.pAgent->UpdateSensoryStimulus(nTargetId, v3TargetPos, bVisible, bHeard);
			zombie.pAgent->Think(nTargetId, fDeltaTime, fDist);

			// 공격 히트 이벤트 수집
			if (zombie.pAgent->ConsumeAttackHit())
				outAttacks.emplace_back(nId, nTargetId);
		}

		// yaw 갱신 — 이동 방향 기반
		Vector3 v3NewPos = zombie.pAgent->GetPosition();
		Vector3 v3XZDelta(v3NewPos.x - zombie.v3PrevPos.x, 0.f, v3NewPos.z - zombie.v3PrevPos.z);
		if (v3XZDelta.LengthSquared() > 0.0001f)
			zombie.fYaw = std::atan2f(v3XZDelta.x, v3XZDelta.z);
		zombie.v3PrevPos = v3NewPos;
	}

	// AIManager 전체 업데이트 (flocking, path search, agent update)
	m_pAIManager->UpdateAll(fDeltaTime);
}

bool ZombieManager::IsVisible(const Vector3& v3From, const Vector3& v3To, float fSightRange) const
{
	float fDist = Vector3::Distance(v3From, v3To);
	if (fDist > fSightRange) return false;

	auto pNavMesh = m_pAIManager ? m_pAIManager->GetNavMesh() : nullptr;
	if (!pNavMesh) return true; // NavMesh 없으면 LOS 체크 생략

	return pNavMesh->IsLineOfSightClear(v3From, v3To);
}
