#include "pch.h"
#include "Zombie.h"
#include "ZombiePool.h"
#include "NodeObject.h"
#include "ZombieAnimationController.h"
#include "BloodEffect.h"

Zombie::Zombie()
{
}

Zombie::~Zombie()
{
	Shutdown();
}

void Zombie::Initialize()
{

	m_pAIAgent = AI->CreateAgent();  // shared_ptr 직접 할당
	if (m_pAIAgent)
		m_pAIAgent->SetMoveSpeed(220.0f);   // 1.1 m/s (플레이어 걷기 140 cm/s보다 약간 느림)

	if (!m_bInitialized) {

		// Model
		//auto pModel = MODEL->Get("Ch33_nonPBR")->CopyObject<NodeObject>();
		auto pModel = GCTX->GetZombieCopy(rand() % 3)->CopyObject<NodeObject>();
		pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
		SetChild(pModel);
		//GetTransform()->Rotate(Vector3::Up, -90.f);

		// AnimationController
		AddComponent<ZombieAnimationController>();
	}

	for (auto& component : m_pComponents) {
		if (component) {
			component->Initialize();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	AddComponent<PlayerCollider>();
	GetComponent<PlayerCollider>()->Initialize();
}

void Zombie::ProcessInput()
{
}

void Zombie::Update()
{
	if (!m_bPoolActive) return;

	for (const auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void Zombie::Shutdown()
{
	// shared_ptr 자동 삭제 (ref count가 0이 되면)
	m_pAIAgent.reset();
}

void Zombie::PoolReset()
{
	m_bPoolActive       = false;
	m_nActiveIndex      = -1;
	m_fHP               = 100.f;
	m_bDying            = false;
	m_bReadyToRemove    = false;
	m_fVerticalVelocity = 0.f;
	m_fMoveSpeedSqXZ    = 0.f;
	m_bWasVisible       = false;
	m_nServerId              = -1;
	m_eServerBehaviorState   = ZBS_Idle;
	m_xmOBBCollided.clear();
	// AI 에이전트 상태는 재활성화 시 SetPosition/SetTarget으로 재설정됨
}

void Zombie::PoolActivate()
{
	m_bPoolActive       = true;
	m_fHP               = 100.f;
	m_bDying            = false;
	m_bReadyToRemove    = false;
	m_fVerticalVelocity = 0.f;
	m_fMoveSpeedSqXZ    = 0.f;
	m_bWasVisible       = false;
	m_xmOBBCollided.clear();
}

void Zombie::PostUpdate()
{
	if (!m_bPoolActive) return;

	// ── 사망 처리 ────────────────────────────────────────────────────────────
	if (m_bDying) {
		auto pAC = GetComponent<ZombieAnimationController>();
		if (pAC && pAC->GetMontage() && pAC->GetMontage()->IsFreezed() && !m_bReadyToRemove)
		{
			m_bReadyToRemove = true;
			if (m_pPool) m_pPool->MarkForRelease(std::static_pointer_cast<Zombie>(shared_from_this()));
		}
		DynamicObject::PostUpdate();
		return;
	}
	if (IsDead()) {
		m_bDying = true;
		auto pAC = GetComponent<ZombieAnimationController>();
		if (pAC && pAC->GetMontage())
			pAC->GetMontage()->PlayMontage("Zombie Death");
		DynamicObject::PostUpdate();
		return;
	}

	auto Target = m_wpTarget.lock();
	if (!m_pAIAgent || !Target)
	{
		DynamicObject::PostUpdate();
		return;
	}

	// ── 감지 처리 + Goal 기반 AI 구동 ─────────────────────────────────────
	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();

	// 공격 애니메이션 재생 중엔 AI 상태 전환 차단
	auto pAC = GetComponent<ZombieAnimationController>();
	bool bAttackMontageActive = pAC && pAC->GetMontage() && pAC->GetMontage()->GetBlendWeight() > 0.f;

	if (!bOnline)
	{
		// 오프라인 전용: FOV/LOS 감지 + Think() + 경보 전파
		// 온라인 모드에서는 서버가 AI를 담당하므로 이 블록 전체를 스킵
		// (100마리 × 60fps LOS 레이캐스트 절감)
		Matrix  worldMatrix = Target->GetWorldMatrix();
		Vector3 v3PlayerPos   = Vector3(worldMatrix._41, worldMatrix._42, worldMatrix._43);

		Vector3 v3ZombiePos = m_pAIAgent->GetPosition();
		float fDist = Vector3::Distance(v3ZombiePos, v3PlayerPos);

		Vector3 v3ToPlayerXZ(v3PlayerPos.x - v3ZombiePos.x, 0.f, v3PlayerPos.z - v3ZombiePos.z);
		float fToPlayerXZLen = v3ToPlayerXZ.Length();
		bool bInFOV = (fDist <= m_fCloseRange);
		if (!bInFOV && fToPlayerXZLen > 0.001f)
			bInFOV = m_v3Forward.Dot(v3ToPlayerXZ / fToPlayerXZLen) >= m_fFOVCosHalf;

		bool bVisible = bInFOV && (fDist <= m_fSightRange) &&
		                AI->GetNavMesh()->IsLineOfSightClear(v3ZombiePos, v3PlayerPos);
		bool bHeard = (fDist <= m_fHearingRange);

		m_pAIAgent->UpdateSensoryStimulus(0, v3PlayerPos, bVisible, bHeard);

		if (!bAttackMontageActive) {
			m_pAIAgent->Think(0, DT, fDist);

			if (m_pAIAgent->ConsumeAttackHit()) {
				if (pAC && pAC->GetMontage())
					pAC->GetMontage()->PlayMontage("Zombie Attack");
			}
		}

		if (bVisible && !m_bWasVisible)
			AI->SpreadAlert(v3ZombiePos, 0, v3PlayerPos, 500.0f);
		m_bWasVisible = bVisible;
	}

	// ── 이동 delta 계산 ─────────────────────────────────────────────────────
	// GetPosition()은 m_mtxWorld(이전 프레임 갱신 기준) → 현재 실제 월드 위치
	Vector3 v3CurrentPos = GetTransform()->GetPosition();
	Vector3 v3AgentPos   = m_pAIAgent->GetPosition();

	Vector3 v3Delta;
	v3Delta.x = v3AgentPos.x - v3CurrentPos.x;   // AI가 계산한 XZ 이동량
	v3Delta.z = v3AgentPos.z - v3CurrentPos.z;
	v3Delta.y = m_fVerticalVelocity * DT;     // 중력에 의한 수직 이동

	// ── 충돌/지형 해소 ──────────────────────────────────────────────────────
	const bool bWasGrounded = m_bGrounded;
	m_bGrounded = false;

	ResolveCollision(v3Delta);

	TerrainHit hit{};
	ResolveTerrain(v3Delta, hit, bWasGrounded);
	if (hit.bGrounded)
	{
		m_bGrounded = true;
		if (m_fVerticalVelocity < 0.f)
			m_fVerticalVelocity = 0.f;
	}

	// ── 중력 누적 ───────────────────────────────────────────────────────────
	ApplyGravity();

	// ── Transform 이동 적용 ─────────────────────────────────────────────────
	GetTransform()->Move(v3Delta, 1.f);

	// ── AI 에이전트 위치를 실제 이동 결과와 동기화 (경로 유지) ─────────────
	// Move()는 m_mtxTransform에 쓰므로 새 위치를 수동 계산
	Vector3 v3NewPos = v3CurrentPos + v3Delta;
	m_pAIAgent->SyncPosition(v3NewPos);

	// ── 이동 속도 / 회전 업데이트 ──────────────────────────────────────────
	Vector3 v3XZDelta(v3Delta.x, 0.f, v3Delta.z);
	float fDTSafe = (DT > 0.f) ? DT : 1.f;

	if (bOnline)
	{
		// 온라인: 실제 델타가 서버 틱(30Hz) 경계에서 0에 수렴해 끊기므로
		// 서버 행동 상태로 Walk/Idle 결정
		bool bMoving = (m_eServerBehaviorState != ZBS_Idle &&
		                m_eServerBehaviorState != ZBS_Alert);
		m_fMoveSpeedSqXZ = bMoving ? 1.f : 0.f;
	}
	else
	{
		m_fMoveSpeedSqXZ = v3XZDelta.LengthSquared() / (fDTSafe * fDTSafe);
	}

	// 오프라인 모드에서만 이동 방향으로 yaw 계산
	// 온라인 모드는 ApplyServerState에서 서버 yaw를 직접 적용
	if (!bOnline && v3XZDelta.LengthSquared() > 0.0001f)
	{
		float fYaw = std::atan2f(v3XZDelta.x, v3XZDelta.z);
		GetTransform()->SetRotation(0.f, fYaw, 0.f);
		m_v3Forward = Vector3(sinf(fYaw), 0.f, cosf(fYaw));
	}

	m_xmOBBCollided.clear();

	// ── 컴포넌트/자식 업데이트 → Transform::Update()로 m_mtxWorld 갱신 ────
	DynamicObject::PostUpdate();
}

Vector3 Zombie::GetPosition() const
{
	if (m_pAIAgent)
		return m_pAIAgent->GetPosition();

	return Vector3::Zero;
}

void Zombie::ApplyGravity()
{
	if (!m_bGrounded)
		m_fVerticalVelocity += m_fGravity * DT;
	else
		m_fVerticalVelocity = 0.f;
}

void Zombie::ResolveCollision(Vector3& outv3Delta)
{
	auto pCollider = GetComponent<PlayerCollider>();
	if (!pCollider)
		return;

	const BoundingCapsule& capsuleWorld = pCollider->GetCapsuleWorld();
	const float fSkin = 0.5f;

	for (auto& xmOBB : m_xmOBBCollided)
	{
		Vector3 v3Normal;
		float fDepth;
		if (!capsuleWorld.Intersects(xmOBB, v3Normal, fDepth))
			continue;
		if (fDepth < fSkin)
			continue;

		float fProjected = outv3Delta.Dot(v3Normal);
		if (fProjected < 0.f)
			outv3Delta -= v3Normal * fProjected;
	}
}

void Zombie::OnBeginCollision(const CollisionResult& collisionResult)
{
	if (!m_bPoolActive) return;
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void Zombie::OnWhileCollision(const CollisionResult& collisionResult)
{
	if (!m_bPoolActive) return;
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void Zombie::OnEndCollision(const CollisionResult& collisionResult)
{
}

void Zombie::OnTraceHit(const RayTraceHitResult& hitResult)
{
	if (IsDead()) {
		return;
	}

	if (hitResult.fDamage > 0.f) {
		TakeDamage(hitResult.fDamage);
		static_cast<IThirdPersonPlayer*>(hitResult.pInstigator)->ShowHitMarker();
	}

	Vector3 v3BloodDir = -hitResult.v3Direction;
	if (v3BloodDir.LengthSquared() > 1e-8f) {
		v3BloodDir.Normalize();
	}

	ParticleEffectSpawnDesc desc;
	{
		desc.v3Position = hitResult.v3ImpactPoint;
		desc.v3Direction = hitResult.v3Direction;
		desc.v3Normal = hitResult.v3ImpactNormal;
		desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
	};

	PARTICLE->Spawn<BloodEffect>(desc);
}

void Zombie::ApplyServerState(float serverX, float serverZ, float serverYaw,
                               ZombieBehaviorState state, float receivedTime)
{
	if (!m_pAIAgent) return;

	// 이미 적용한 상태면 스킵 — 매 프레임 SetDirectPath 리셋 방지
	if (receivedTime <= m_fLastAppliedTime) return;
	m_fLastAppliedTime = receivedTime;

	Vector3 v3AgentPos = m_pAIAgent->GetPosition();
	Vector3 v3TargetPos(serverX, v3AgentPos.y, serverZ);

	float fErrXZSq = (v3AgentPos.x - serverX) * (v3AgentPos.x - serverX)
	               + (v3AgentPos.z - serverZ) * (v3AgentPos.z - serverZ);

	// 5m 이상 벗어나면 즉시 스냅
	static constexpr float SNAP_THRESHOLD_SQ = 500.f * 500.f;
	if (fErrXZSq > SNAP_THRESHOLD_SQ)
		m_pAIAgent->SyncPosition(v3TargetPos);
	else if (fErrXZSq > 1.f)
		m_pAIAgent->SetDirectPath(v3TargetPos);

	// 서버 yaw 직접 적용
	GetTransform()->SetRotation(0.f, serverYaw, 0.f);
	m_v3Forward = Vector3(sinf(serverYaw), 0.f, cosf(serverYaw));

	m_eServerBehaviorState = state;
}

void Zombie::TriggerAttackHit()
{
	auto pTarget = m_wpTarget.lock();
	if (!pTarget)
		return;

	// Notify 시점에 여전히 공격 사거리 이내일 때만 데미지 적용
	Matrix worldMatrix = pTarget->GetWorldMatrix();
	Vector3 v3TargetPos(worldMatrix._41, worldMatrix._42, worldMatrix._43);
	float fDist = Vector3::Distance(m_pAIAgent->GetPosition(), v3TargetPos);
	if (fDist <= m_fCloseRange)
		pTarget->TakeDamage(10.f);
}
