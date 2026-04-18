#include "pch.h"
#include "Zombie.h"
#include "ZombiePool.h"
#include "NodeObject.h"
#include "ZombieAnimationController.h"
// #include "Player.h"           ← 프로젝트에 맞게 include
// #include "AIManagerWrapper.h" ← 싱글톤 래퍼

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
		m_pAIAgent->SetMoveSpeed(110.0f);   // 1.1 m/s (플레이어 걷기 140 cm/s보다 약간 느림)

	if (!m_bInitialized) {

		// Model
		auto pModel = MODEL->Get("Ch33_nonPBR")->CopyObject<NodeObject>();
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
	Matrix  worldMatrix = Target->GetWorldMatrix();
	Vector3 v3PlayerPos   = Vector3(worldMatrix._41, worldMatrix._42, worldMatrix._43);

	Vector3 v3ZombiePos = m_pAIAgent->GetPosition();
	float fDist = Vector3::Distance(v3ZombiePos, v3PlayerPos);

	// ── FOV 시야각 체크 (전방 ±60°, 근거리 150cm 이하는 무시) ──────────────
	Vector3 v3ToPlayerXZ(v3PlayerPos.x - v3ZombiePos.x, 0.f, v3PlayerPos.z - v3ZombiePos.z);
	float fToPlayerXZLen = v3ToPlayerXZ.Length();
	bool bInFOV = (fDist <= m_fCloseRange);
	if (!bInFOV && fToPlayerXZLen > 0.001f)
		bInFOV = m_v3Forward.Dot(v3ToPlayerXZ / fToPlayerXZLen) >= m_fFOVCosHalf;

	// LOS 레이캐스트: FOV 안에 있고, 거리 안에 있고, 벽에 막히지 않으면 시야 감지
	bool bVisible = bInFOV && (fDist <= m_fSightRange) &&
	                AI->GetNavMesh()->IsLineOfSightClear(v3ZombiePos, v3PlayerPos);
	bool bHeard = (fDist <= m_fHearingRange);

	m_pAIAgent->UpdateSensoryStimulus(0, v3PlayerPos, bVisible, bHeard);

	// 공격 애니메이션 재생 중엔 AI 상태 전환 차단
	auto pAC = GetComponent<ZombieAnimationController>();
	bool bAttackMontageActive = pAC && pAC->GetMontage() && pAC->GetMontage()->GetBlendWeight() > 0.f;

	if (!bAttackMontageActive) {
		m_pAIAgent->Think(0, DT, fDist);

		// GoalAttack 쿨다운 완료 시 공격 애니메이션 재생 (실제 데미지는 Notify에서)
		if (m_pAIAgent->ConsumeAttackHit()) {
			if (pAC && pAC->GetMontage())
				pAC->GetMontage()->PlayMontage("Zombie Attack");
		}
	}

	// 최초 발견 순간(rising edge)에만 주변 좀비들에게 경보 전파
	if (bVisible && !m_bWasVisible)
		AI->SpreadAlert(v3ZombiePos, 0, v3PlayerPos, 500.0f);
	m_bWasVisible = bVisible;

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

	// ── 이동 방향으로 좀비 회전 ─────────────────────────────────────────────
	Vector3 v3XZDelta(v3Delta.x, 0.f, v3Delta.z);
	float fDTSafe = (DT > 0.f) ? DT : 1.f;
	m_fMoveSpeedSqXZ = v3XZDelta.LengthSquared() / (fDTSafe * fDTSafe);

	if (v3XZDelta.LengthSquared() > 0.0001f)
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
