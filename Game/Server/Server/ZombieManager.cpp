#include "pch.h"
#include "ZombieManager.h"
#include "ServerBoundingCapsule.h"

#define JSON_HAS_RANGES 0
#include <Includes/nlohmann_json/json.hpp>

bool ZombieManager::LoadAttackAnimDuration(const std::string& strAnimBinPath)
{
	std::ifstream in(strAnimBinPath, std::ios::binary);
	if (!in) {
		//std::cout << "[ZombieManager] Attack anim not found: " << strAnimBinPath << "\n";
		return false;
	}

	std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)),
	                          std::istreambuf_iterator<char>());

	nlohmann::json j = nlohmann::json::from_bson(buf);

	if (!j.contains("Animations") || j["Animations"].empty())
		return false;

	float fDuration = j["Animations"][0]["Duration"].get<float>();
	m_fAttackAnimDuration = fDuration + m_fBlendOutTime;

	//std::cout << "[ZombieManager] Attack anim duration=" << fDuration
	//          << "s + blendOut=" << m_fBlendOutTime
	//          << "s = " << m_fAttackAnimDuration << "s\n";
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

	//std::cout << "[ZombieManager] 좀비 스폰 id=" << nId
	//          << " pos=(" << v3SpawnPos.x << "," << v3SpawnPos.z << ")\n";
	return nId;
}

int ZombieManager::LoadSpawnPoints(const std::string& strSpawnJsonPath)
{
	m_SpawnPoints.clear();

	std::ifstream in(strSpawnJsonPath);
	if (!in) {
		std::cout << "[ZombieManager] Spawn point file not found: " << strSpawnJsonPath << "\n";
		return 0;
	}

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("ZombieSpawnPoints"))
		return 0;

	for (const auto& jSpawn : j["ZombieSpawnPoints"])
	{
		const auto& wm = jSpawn["Transform"]["WorldMatrix"];
		// 행 우선 4x4 — translation은 인덱스 12,13,14 (_41,_42,_43)
		Vector3 v3Pos(wm[12].get<float>(), wm[13].get<float>(), wm[14].get<float>());
		m_SpawnPoints.push_back(v3Pos);
	}

	std::cout << "[ZombieManager] 스폰 포인트 로드: " << m_SpawnPoints.size() << "개\n";
	return static_cast<int>(m_SpawnPoints.size());
}

Vector3 ZombieManager::GetRandomSpawnPoint() const
{
	if (m_SpawnPoints.empty())
		return Vector3::Zero;   // 폴백: SpawnZombie가 랜덤 NavMesh로 처리
	return m_SpawnPoints[rand() % m_SpawnPoints.size()];
}

Vector3 ZombieManager::GetSpawnPointNear(const std::vector<Vector3>& playerPositions) const
{
	if (m_SpawnPoints.empty())   return Vector3::Zero;
	if (playerPositions.empty()) return GetRandomSpawnPoint();

	// 거리 window: 너무 가깝(눈앞)지도, 너무 멀(이미 지난/못 따라옴)지도 않은 포인트만 사용.
	constexpr float fMinDist = 800.f;    // 8m  — 플레이어 코앞 스폰 방지
	constexpr float fMaxDist = 20000.f;   // 60m — 이 밖은 "이미 지난/너무 먼" 포인트로 간주

	std::vector<const Vector3*> candidates;
	candidates.reserve(m_SpawnPoints.size());

	const Vector3* pClosest = nullptr;
	float fClosestDist = FLT_MAX;

	for (const auto& sp : m_SpawnPoints) {
		// 가장 가까운 플레이어까지 거리
		float fNearest = FLT_MAX;
		for (const auto& pp : playerPositions) {
			float d = Vector3::Distance(sp, pp);
			if (d < fNearest) fNearest = d;
		}
		if (fNearest >= fMinDist && fNearest <= fMaxDist)
			candidates.push_back(&sp);
		if (fNearest < fClosestDist) { fClosestDist = fNearest; pClosest = &sp; }
	}

	if (!candidates.empty())
		return *candidates[rand() % candidates.size()];

	// window 내 후보가 없으면 가장 가까운 포인트로 폴백 (먼 뒤쪽 랜덤 방지)
	return pClosest ? *pClosest : GetRandomSpawnPoint();
}

void ZombieManager::DespawnZombie(int nId)
{
	auto it = m_Zombies.find(nId);
	if (it == m_Zombies.end()) return;

	m_Zombies.erase(it);
	//std::cout << "[ZombieManager] 좀비 디스폰 id=" << nId << "\n";
}

void ZombieManager::Tick(float fDeltaTime,
                         const std::unordered_map<int, Vector3>& playerPositions,
                         std::vector<std::pair<int,int>>& outAttacks,
                         std::vector<std::pair<int,int>>& outAttackAnims)
{
	if (!m_pAIManager) return;

	static int nDebugCounter = 0;
	bool bDebugPrint = (++nDebugCounter % 150 == 0); // 5초마다 (30Hz * 5)

	//if (bDebugPrint)
		//std::cout << "[ZombieTick] players=" << playerPositions.size()
		//          << " zombies=" << m_Zombies.size() << "\n";

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
			// 시야(FOV)/LOS 제거 — 반경(m_fSightRange) 내에 있으면 무조건 인지.
			bool bVisible = (fDist <= m_fSightRange);
			bool bHeard   = (fDist <= m_fHearingRange);
			zombie.pAgent->UpdateSensoryStimulus(0, v3TargetPos, bVisible, bHeard);

			//if (bDebugPrint && nId == 0)
			//	std::cout << "  zombie[0] dist=" << fDist
			//	          << " visible=" << bVisible
			//	          << " heard=" << bHeard
			//	          << " state=" << (int)zombie.pAgent->GetBehaviorState() << "\n";
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
				//static const char* sNames[] = {"Idle","Wander","Alert","Invest","Chase","Attack"};
				//std::cout << "[Z0] " << sNames[nPrevState] << " -> " << sNames[nNewState]
				//          << " dist=" << fDist
				//          << " pathState=" << static_cast<int>(zombie.pAgent->GetPathState()) << "\n";
			}

			if (zombie.pAgent->ConsumeAttackHit() && nTargetId >= 0) {
				zombie.fAttackTimer = m_fAttackAnimDuration;
				zombie.fAttackDamageDelay = m_fAttackDamageNotifyDelay;
				zombie.nAttackTargetId = nTargetId;
				outAttackAnims.emplace_back(nId, nTargetId); // 즉시 몽타주 시작
			}
		}

		// yaw 갱신 — 이동 방향 기반 + 회전 속도 제한 스무딩
		// 틱당 delta(~10cm) 생값 atan2는 flock 힘의 미세 진동에 yaw가 널뛰고,
		// 200ms 샘플링을 거쳐 클라이언트에서 빙글빙글 도는 것처럼 보간된다.
		// AI DLL 경로 방향 스무딩(g_fTurnSpeed)과 동일한 5 rad/s 제한.
		Vector3 v3NewPos = zombie.pAgent->GetPosition();
		Vector3 v3XZDelta(v3NewPos.x - zombie.v3PrevPos.x, 0.f, v3NewPos.z - zombie.v3PrevPos.z);
		if (v3XZDelta.LengthSquared() > 1.f) // 틱당 1cm 초과 이동 시만 (정지 시 노이즈 필터)
		{
			float fTargetYaw = std::atan2f(v3XZDelta.x, v3XZDelta.z);
			float fYawDiff = fTargetYaw - zombie.fYaw;
			fYawDiff = std::fmodf(fYawDiff + DirectX::XM_PI, DirectX::XM_2PI);
			if (fYawDiff < 0.f) fYawDiff += DirectX::XM_2PI;
			fYawDiff -= DirectX::XM_PI;

			const float fMaxStep = 5.f * fDeltaTime;
			zombie.fYaw += std::clamp(fYawDiff, -fMaxStep, fMaxStep);
			zombie.fYaw = std::fmodf(zombie.fYaw + DirectX::XM_PI, DirectX::XM_2PI);
			if (zombie.fYaw < 0.f) zombie.fYaw += DirectX::XM_2PI;
			zombie.fYaw -= DirectX::XM_PI;
		}

		// Chase 상태인데 이동 없으면 위치 출력
		if (zombie.pAgent->GetBehaviorState() == AIBehaviorState::Chasing &&
		    v3XZDelta.LengthSquared() <= 0.0001f)
		{
			zombie.fStuckTimer += fDeltaTime;
			if (zombie.fStuckTimer >= 1.f) // 1초 이상 멈춰 있을 때만 출력
			{
				/*printf("[Stuck] zombie=%d  pos=(%.0f,%.0f,%.0f)  pathState=%d\n",
				       nId,
				       v3NewPos.x, v3NewPos.y, v3NewPos.z,
				       (int)zombie.pAgent->GetPathState());
				zombie.fStuckTimer = 0.f;*/
			}
		}
		else
		{
			zombie.fStuckTimer = 0.f;
		}

		zombie.v3PrevPos = v3NewPos;
	}

	// AIManager 전체 업데이트 (flocking, path search, agent update)
	m_pAIManager->UpdateAll(fDeltaTime);

	// NavMesh 클램핑 — 서버에는 중력/충돌이 없으므로 매 틱 Y 보정
	// 클라이언트와 동일하게 임계값 초과 시에만 클램핑.
	// 무조건 클램핑하면 Boids가 조금 밀어낼 때마다 즉시 원위치되어
	// PathState=Moving인데 순이동 0인 stuck 현상 발생.
	// XZ: 30cm 이상 벗어날 때만 클램핑 (Boids 소량 편차로 인한 stuck 방지)
	// Y:  항상 NavMesh 높이로 보정 (서버에는 중력 없으므로 Y 드리프트 방지)
	static constexpr float CLAMP_THRESHOLD_XZ = 30.f; // cm
	auto pNavMesh = m_pAIManager->GetNavMesh();
	if (pNavMesh) {
		for (auto& [nId, zombie] : m_Zombies)
		{
			if (!zombie.bAlive || !zombie.pAgent) continue;

			Vector3 v3Pos    = zombie.pAgent->GetPosition();
			Vector3 v3Clamped = pNavMesh->GetNearestPointOnNavMesh(v3Pos);

			float fXZDist = sqrtf(
				(v3Pos.x - v3Clamped.x) * (v3Pos.x - v3Clamped.x) +
				(v3Pos.z - v3Clamped.z) * (v3Pos.z - v3Clamped.z));

			if (fXZDist > CLAMP_THRESHOLD_XZ)
			{
				// XZ도 크게 벗어남 → 전체 클램핑
				zombie.pAgent->SyncPosition(v3Clamped);
			}
			else
			{
				// XZ는 허용 범위 → Y만 NavMesh 높이로 보정
				Vector3 v3YFixed = v3Pos;
				v3YFixed.y = v3Clamped.y;
				zombie.pAgent->SyncPosition(v3YFixed);
			}
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

bool ZombieManager::IsVisible(const Vector3& v3From, const Vector3& v3To,
                              const Vector3& v3Forward, float fSightRange) const
{
	float fDist = Vector3::Distance(v3From, v3To);
	if (fDist > fSightRange) return false;

	// FOV 콘: 근거리(m_fCloseRange)는 시야각 무시, 그 밖은 전방 ±60° 이내일 때만
	// (클라이언트 Zombie::PostUpdate 오프라인 로직과 동일)
	bool bInFOV = (fDist <= m_fCloseRange);
	if (!bInFOV)
	{
		Vector3 v3ToTargetXZ(v3To.x - v3From.x, 0.f, v3To.z - v3From.z);
		float fLenXZ = v3ToTargetXZ.Length();
		if (fLenXZ > 0.001f)
			bInFOV = v3Forward.Dot(v3ToTargetXZ / fLenXZ) >= m_fFOVCosHalf;
	}
	if (!bInFOV) return false;

	auto pNavMesh = m_pAIManager ? m_pAIManager->GetNavMesh() : nullptr;
	if (!pNavMesh) return true; // NavMesh 없으면 LOS 체크 생략

	return pNavMesh->IsLineOfSightClear(v3From, v3To);
}
