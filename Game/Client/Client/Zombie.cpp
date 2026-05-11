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
	m_fLastAppliedTime       = -1.f;
	m_fCurrentYaw            = 0.f;
	m_fTargetYaw             = 0.f;
	m_fInterpDelay           = 0.1f;
	m_Snapshots.clear();
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
	// GetPosition() = m_mtxWorld, SetPosition/Move = m_mtxTransform
	// 루트 좀비: m_mtxFrameRelative = identity → m_mtxWorld = m_mtxTransform
	// 따라서 GetPosition()과 Move/SetPosition은 같은 좌표계
	Vector3 v3CurrentPos = GetTransform()->GetPosition();
	Vector3 v3Delta;

	float fInterpYaw = m_fCurrentYaw;

	// 디버그용
	int    nDbgSnapCount  = 0;
	float  fDbgRenderTime = 0.f;
	float  fDbgFrontTime  = 0.f;
	float  fDbgBackTime   = 0.f;
	int    nDbgMode       = 0; // 0=no snap, 1=bracket, 2=before, 3=extrap, 4=single
	float  fDbgT          = 0.f;
	Vector3 v3DbgInterpPos = Vector3::Zero;
	Vector3 v3DbgDelta     = Vector3::Zero;

	if (bOnline)
	{
		// ── 온라인: 스냅샷 시간 기반 보간 ────────────────────────────────────
		Vector3 v3InterpPos = v3CurrentPos;

		nDbgSnapCount = static_cast<int>(m_Snapshots.size());

		if (!m_Snapshots.empty())
		{
			float fRenderTime = NetworkManager::GetNetTimeSec() - m_fInterpDelay;
			fDbgRenderTime = fRenderTime;
			fDbgFrontTime  = m_Snapshots.front().fTime;
			fDbgBackTime   = m_Snapshots.back().fTime;

			bool bFound = false;
			for (int i = static_cast<int>(m_Snapshots.size()) - 1; i >= 1; --i)
			{
				const auto& s0 = m_Snapshots[i - 1];
				const auto& s1 = m_Snapshots[i];
				if (s0.fTime <= fRenderTime && fRenderTime <= s1.fTime)
				{
					float fSpan = s1.fTime - s0.fTime;
					float t = (fSpan > 0.f) ? (fRenderTime - s0.fTime) / fSpan : 1.f;
					v3InterpPos = Vector3::Lerp(s0.v3Pos, s1.v3Pos, t);

					float fYawDiff = s1.fYaw - s0.fYaw;
					fYawDiff = std::fmodf(fYawDiff + XM_PI, XM_2PI) - XM_PI;
					fInterpYaw = s0.fYaw + fYawDiff * t;

					bFound = true;
					nDbgMode = 1;
					fDbgT = t;
					break;
				}
			}

			if (!bFound)
			{
				if (fRenderTime < m_Snapshots.front().fTime)
				{
					v3InterpPos = m_Snapshots.front().v3Pos;
					fInterpYaw  = m_Snapshots.front().fYaw;
					nDbgMode = 2;
				}
				else if (m_Snapshots.size() >= 2)
				{
					const auto& s0 = m_Snapshots[m_Snapshots.size() - 2];
					const auto& s1 = m_Snapshots.back();
					float fSpan = s1.fTime - s0.fTime;
					if (fSpan > 0.f)
					{
						Vector3 v3Vel = (s1.v3Pos - s0.v3Pos) * (1.f / fSpan);
						float fExtrap = std::min(fRenderTime - s1.fTime, 0.5f);
						v3InterpPos = s1.v3Pos + v3Vel * fExtrap;
					}
					else
					{
						v3InterpPos = s1.v3Pos;
					}
					fInterpYaw = s1.fYaw;
					nDbgMode = 3;
				}
				else
				{
					v3InterpPos = m_Snapshots.back().v3Pos;
					fInterpYaw  = m_Snapshots.back().fYaw;
					nDbgMode = 4;
				}
			}
		}

		v3DbgInterpPos = v3InterpPos;

		// XZ: 보간, Y: 중력
		v3Delta.x = v3InterpPos.x - v3CurrentPos.x;
		v3Delta.z = v3InterpPos.z - v3CurrentPos.z;
		v3Delta.y = m_fVerticalVelocity * DT;

		v3DbgDelta = v3Delta;

		// 애니메이션
		bool bMoving = (m_eServerBehaviorState != ZBS_Idle &&
		                m_eServerBehaviorState != ZBS_Alert);
		m_fMoveSpeedSqXZ = bMoving ? 1.f : 0.f;
	}
	else
	{
		// ── 오프라인: AI agent 경로 시스템 ────────────────────────────────────
		Vector3 v3AgentPos = m_pAIAgent->GetPosition();
		v3Delta.x = v3AgentPos.x - v3CurrentPos.x;
		v3Delta.z = v3AgentPos.z - v3CurrentPos.z;
		v3Delta.y = m_fVerticalVelocity * DT;
	}

	// ── 충돌/지형 해소 (온라인·오프라인 공통) ────────────────────────────────
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

	// ── AI 에이전트 위치 동기화 ──────────────────────────────────────────────
	Vector3 v3NewPos = v3CurrentPos + v3Delta;
	m_pAIAgent->SyncPosition(v3NewPos);

	// ── 회전 ────────────────────────────────────────────────────────────────
	if (bOnline)
	{
		m_fCurrentYaw = fInterpYaw;
		GetTransform()->SetRotation(0.f, m_fCurrentYaw, 0.f);
		m_v3Forward = Vector3(sinf(m_fCurrentYaw), 0.f, cosf(m_fCurrentYaw));
	}
	else
	{
		Vector3 v3XZDelta(v3Delta.x, 0.f, v3Delta.z);
		float fDTSafe = (DT > 0.f) ? DT : 1.f;
		m_fMoveSpeedSqXZ = v3XZDelta.LengthSquared() / (fDTSafe * fDTSafe);

		if (v3XZDelta.LengthSquared() > 0.0001f)
		{
			float fYaw = std::atan2f(v3XZDelta.x, v3XZDelta.z);
			GetTransform()->SetRotation(0.f, fYaw, 0.f);
			m_v3Forward = Vector3(sinf(fYaw), 0.f, cosf(fYaw));
		}
	}

	m_xmOBBCollided.clear();

	// ── 컴포넌트/자식 업데이트 → Transform::Update()로 m_mtxWorld 갱신 ────
	DynamicObject::PostUpdate();

	// ── 디버그 ImGui (첫 번째 서버 좀비만) ───────────────────────────────────
	if (bOnline && m_nServerId == 0)
	{
		static const char* sModes[] = { "NoSnap", "Bracket", "Before", "Extrap", "Single" };
		ImGui::Begin("Zombie Interp Debug");
		ImGui::Text("Snapshots: %d", nDbgSnapCount);
		ImGui::Text("RenderTime: %.4f", fDbgRenderTime);
		ImGui::Text("Front time: %.4f", fDbgFrontTime);
		ImGui::Text("Back  time: %.4f", fDbgBackTime);
		ImGui::Text("Gap (back-front): %.4f", fDbgBackTime - fDbgFrontTime);
		ImGui::Text("Delay (back-render): %.4f", fDbgBackTime - fDbgRenderTime);
		ImGui::Text("InterpDelay: %.4f", m_fInterpDelay);
		ImGui::Text("Mode: %s", sModes[nDbgMode]);
		ImGui::Text("Lerp t: %.4f", fDbgT);
		ImGui::Separator();
		ImGui::Text("CurrentPos: %.1f, %.1f, %.1f", v3CurrentPos.x, v3CurrentPos.y, v3CurrentPos.z);
		ImGui::Text("InterpPos:  %.1f, %.1f, %.1f", v3DbgInterpPos.x, v3DbgInterpPos.y, v3DbgInterpPos.z);
		ImGui::Text("Delta:      %.2f, %.2f, %.2f", v3DbgDelta.x, v3DbgDelta.y, v3DbgDelta.z);
		ImGui::Text("DeltaXZ:    %.2f", sqrtf(v3DbgDelta.x*v3DbgDelta.x + v3DbgDelta.z*v3DbgDelta.z));
		ImGui::End();
	}
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
	const float fGround = 0.7f;
	const float fSnapDistance = 1.0f;

	for (auto& xmOBB : m_xmOBBCollided)
	{
		Vector3 v3Normal;
		float fDepth;
		if (!capsuleWorld.Intersects(xmOBB, v3Normal, fDepth))
			continue;
		if (fDepth < fSkin)
			continue;

		if (v3Normal.y > fGround && outv3Delta.y <= 0.f)
		{
			// 바닥 접촉 확정
			m_bGrounded = true;

			if (m_fVerticalVelocity < 0.f)
				m_fVerticalVelocity = 0.f;

			// Penetration Correction
			if (fDepth > fSnapDistance)
			{
				float fPush = std::min(fDepth + fSkin, 5.f);
				outv3Delta += v3Normal * fPush;
			}

			// Slope Projection
			float fProjected = outv3Delta.Dot(v3Normal);
			if (fProjected < 0.f)
				outv3Delta -= v3Normal * fProjected;
		}
		else
		{
			// 벽/천장 충돌 — 이동 방향만 차단
			float fProjected = outv3Delta.Dot(v3Normal);
			if (fProjected < 0.f)
				outv3Delta -= v3Normal * fProjected;
		}
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

	// 이미 저장한 스냅샷이면 스킵 (매 프레임 동일 상태 반복 방지)
	if (!m_Snapshots.empty() && receivedTime <= m_fLastAppliedTime)
		return;
	m_fLastAppliedTime = receivedTime;

	// 스냅샷 저장 — 메인 스레드 현재 시간 기준 (PostUpdate와 동일 시간축)
	float fNow = NetworkManager::GetNetTimeSec();

	// 서버 전송 간격 측정 → 보간 딜레이 자동 조절
	if (!m_Snapshots.empty())
	{
		float fInterval = fNow - m_Snapshots.back().fTime;
		if (fInterval > 0.01f)
			m_fInterpDelay = fInterval * 1.5f; // 간격의 1.5배로 여유
	}

	float fY = m_pAIAgent->GetPosition().y;
	ZombieSnapshot snap;
	snap.v3Pos = Vector3(serverX, fY, serverZ);
	snap.fYaw  = serverYaw;
	snap.fTime = fNow;

	m_Snapshots.push_back(snap);
	while (m_Snapshots.size() > MAX_SNAPSHOTS)
		m_Snapshots.pop_front();

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
