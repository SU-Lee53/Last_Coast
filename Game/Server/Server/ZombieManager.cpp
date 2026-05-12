#include "pch.h"
#include "ZombieManager.h"
#include "ServerBoundingCapsule.h"

#define JSON_HAS_RANGES 0
#include <Includes/nlohmann_json/json.hpp>

bool ZombieManager::LoadAttackAnimDuration(const std::string& strAnimBinPath)
{
	std::ifstream in(strAnimBinPath, std::ios::binary);
	if (!in) {
		std::cout << "[ZombieManager] Attack anim not found: " << strAnimBinPath << "\n";
		return false;
	}

	std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)),
	                          std::istreambuf_iterator<char>());

	nlohmann::json j = nlohmann::json::from_bson(buf);

	if (!j.contains("Animations") || j["Animations"].empty())
		return false;

	float fDuration = j["Animations"][0]["Duration"].get<float>();
	m_fAttackAnimDuration = fDuration + m_fBlendOutTime;

	std::cout << "[ZombieManager] Attack anim duration=" << fDuration
	          << "s + blendOut=" << m_fBlendOutTime
	          << "s = " << m_fAttackAnimDuration << "s\n";
	return true;
}

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

int ZombieManager::SpawnZombie(Vector3 SpawnPos)
{
	if (!m_pAIManager)
		return -1;

	auto pNavMesh = m_pAIManager->GetNavMesh();
	if (!pNavMesh)
		return -1;
	Vector3 v3SpawnPos;
	if (SpawnPos == Vector3::Zero) {
		v3SpawnPos = pNavMesh->GetRandomPoint();
	}
	else {
		v3SpawnPos = SpawnPos;
	}

	auto pAgent = m_pAIManager->CreateAgent();
	if (!pAgent)
		return -1;

	pAgent->SetPosition(v3SpawnPos);
	pAgent->SetMoveSpeed(220.f); // 클라이언트와 동일 (2.2m/s)

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
                         std::vector<std::pair<int,int>>& outAttacks,
                         std::vector<std::pair<int,int>>& outAttackAnims)
{
	if (!m_pAIManager) return;

	static int nDebugCounter = 0;
	bool bDebugPrint = (++nDebugCounter % 150 == 0); // 5초마다 (30Hz * 5)

	if (bDebugPrint)
		std::cout << "[ZombieTick] players=" << playerPositions.size()
		          << " zombies=" << m_Zombies.size() << "\n";

	for (auto& [nId, zombie] : m_Zombies)
	{
		if (!zombie.bAlive || !zombie.pAgent) continue;

		Vector3 v3ZombiePos = zombie.pAgent->GetPosition();

		// 타겟 고정 타이머 감소
		if (zombie.fTargetLockTimer > 0.f)
			zombie.fTargetLockTimer -= fDeltaTime;

		// 타겟 선택: 고정 타이머 중이면 기존 타겟 유지, 아니면 가장 가까운 플레이어
		int     nTargetId   = -1;
		float   fMinDistSq  = FLT_MAX;
		Vector3 v3TargetPos = {};

		if (zombie.fTargetLockTimer > 0.f && zombie.nLockedTargetId >= 0) {
			// 고정된 타겟이 아직 접속 중이면 유지
			auto itLocked = playerPositions.find(zombie.nLockedTargetId);
			if (itLocked != playerPositions.end()) {
				nTargetId   = zombie.nLockedTargetId;
				v3TargetPos = itLocked->second;
				fMinDistSq  = Vector3::DistanceSquared(v3ZombiePos, v3TargetPos);
			}
		}

		// 고정 타겟 없거나 만료/접속 해제 → 가장 가까운 플레이어 재탐색
		if (nTargetId < 0) {
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

			// 새 타겟 고정
			if (nTargetId >= 0) {
				zombie.nLockedTargetId  = nTargetId;
				zombie.fTargetLockTimer = m_fTargetLockDuration;
			}
		}

		float fDist = (nTargetId >= 0) ? std::sqrtf(fMinDistSq) : FLT_MAX;

		// entity ID는 항상 0 고정 (AI Evaluator들이 최초 생성 시 ID를 고정하므로)
		// 가장 가까운 플레이어 위치를 entity 0에 주입
		if (nTargetId >= 0)
		{
			bool bVisible = IsVisible(v3ZombiePos, v3TargetPos, m_fSightRange);
			bool bHeard   = (fDist <= m_fHearingRange);
			zombie.pAgent->UpdateSensoryStimulus(0, v3TargetPos, bVisible, bHeard);

			if (bDebugPrint && nId == 0)
				std::cout << "  zombie[0] dist=" << fDist
				          << " visible=" << bVisible
				          << " heard=" << bHeard
				          << " state=" << (int)zombie.pAgent->GetBehaviorState() << "\n";
		}

		// 공격 데미지 딜레이 처리 (애니메이션 Notify 타이밍 모사)
		if (zombie.fAttackDamageDelay > 0.f) {
			zombie.fAttackDamageDelay -= fDeltaTime;
			if (zombie.fAttackDamageDelay <= 0.f && zombie.nAttackTargetId >= 0) {
				// Notify 시점에 사거리 이내인지 확인 (클라이언트 TriggerAttackHit과 동일)
				auto itTarget = playerPositions.find(zombie.nAttackTargetId);
				if (itTarget != playerPositions.end()) {
					float fAttackDist = Vector3::Distance(v3ZombiePos, itTarget->second);
					if (fAttackDist <= 150.f) // m_fCloseRange
						outAttacks.emplace_back(nId, zombie.nAttackTargetId);
				}
				zombie.nAttackTargetId = -1;
			}
		}

		// 공격 애니메이션 대기 중엔 Think 스킵
		if (zombie.fAttackTimer > 0.f) {
			zombie.fAttackTimer -= fDeltaTime;
		}
		else {
			int nPrevState = static_cast<int>(zombie.pAgent->GetBehaviorState());
			zombie.pAgent->Think(0, fDeltaTime, fDist);
			int nNewState = static_cast<int>(zombie.pAgent->GetBehaviorState());

			if (nId == 0 && nPrevState != nNewState) {
				static const char* sNames[] = {"Idle","Wander","Alert","Invest","Chase","Attack"};
				std::cout << "[Z0] " << sNames[nPrevState] << " -> " << sNames[nNewState]
				          << " dist=" << fDist
				          << " pathState=" << static_cast<int>(zombie.pAgent->GetPathState()) << "\n";
			}

			if (zombie.pAgent->ConsumeAttackHit() && nTargetId >= 0) {
				zombie.fAttackTimer = m_fAttackAnimDuration;
				zombie.fAttackDamageDelay = m_fAttackDamageNotifyDelay;
				zombie.nAttackTargetId = nTargetId;
				outAttackAnims.emplace_back(nId, nTargetId); // 즉시 몽타주 시작
			}
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

	// NavMesh 클램핑 — 서버에는 중력/충돌이 없으므로 매 틱 Y 보정
	auto pNavMesh = m_pAIManager->GetNavMesh();
	if (pNavMesh) {
		for (auto& [nId, zombie] : m_Zombies)
		{
			if (!zombie.bAlive || !zombie.pAgent) continue;

			Vector3 v3Pos = zombie.pAgent->GetPosition();
			Vector3 v3Clamped = pNavMesh->GetNearestPointOnNavMesh(v3Pos);
			zombie.pAgent->SyncPosition(v3Clamped);
		}
	}

	// 죽은 좀비 정리 — 클라이언트 사망 애니메이션 대기 후 제거
	std::vector<int> deadIds;
	for (auto& [nId, zombie] : m_Zombies)
	{
		if (zombie.bAlive) continue;

		zombie.fDeadTimer += fDeltaTime;
		if (zombie.fDeadTimer >= m_fDeadCleanupDelay)
			deadIds.push_back(nId);
	}
	for (int nId : deadIds)
		m_Zombies.erase(nId);
}

bool ZombieManager::RayTestZombies(const Vector3& v3Origin, const Vector3& v3Dir,
                                   float fMaxDist, int& outZombieId, float& outDist) const
{
	outZombieId = -1;
	outDist = fMaxDist;

	XMVECTOR xmOrigin = XMLoadFloat3(&v3Origin);
	XMVECTOR xmDir    = XMLoadFloat3(&v3Dir);

	bool bHit = false;
	for (const auto& [nId, zombie] : m_Zombies)
	{
		if (!zombie.bAlive || !zombie.pAgent) continue;

		Vector3 v3Pos = zombie.pAgent->GetPosition();
		v3Pos.y += m_fZombieCapsuleYOffset;

		ServerBoundingCapsule capsule;
		capsule.v3Center    = v3Pos;
		capsule.fHalfHeight = m_fZombieCapsuleHalfHeight;
		capsule.fRadius     = m_fZombieCapsuleRadius;

		float fDist = 0.f;
		if (!capsule.Intersects(v3Origin, v3Dir, fDist))
			continue;

		if (fDist < 0.f || fDist > outDist)
			continue;

		outDist = fDist;
		outZombieId = nId;
		bHit = true;
	}

	return bHit;
}

bool ZombieManager::ApplyDamageToZombie(int nZombieId, float fDamage)
{
	auto it = m_Zombies.find(nZombieId);
	if (it == m_Zombies.end()) return false;

	ServerZombie& zombie = it->second;
	if (!zombie.bAlive) return false;

	zombie.fHP -= fDamage;
	if (zombie.fHP <= 0.f)
	{
		zombie.bAlive = false;
		return true; // 사망
	}
	return false;
}

bool ZombieManager::IsVisible(const Vector3& v3From, const Vector3& v3To, float fSightRange) const
{
	float fDist = Vector3::Distance(v3From, v3To);
	if (fDist > fSightRange) return false;

	auto pNavMesh = m_pAIManager ? m_pAIManager->GetNavMesh() : nullptr;
	if (!pNavMesh) return true; // NavMesh 없으면 LOS 체크 생략

	return pNavMesh->IsLineOfSightClear(v3From, v3To);
}
