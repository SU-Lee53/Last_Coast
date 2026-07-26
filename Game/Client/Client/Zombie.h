#pragma once
#include "DynamicObject.h"

class IPlayer;
class ZombiePool;

class Zombie : public DynamicObject
{
public:
	Zombie();
	~Zombie();

	virtual void Initialize() override;
	virtual void ProcessInput() override;

	virtual void Update() override;
	virtual void PostUpdate() override;

	void Shutdown();
	void SetTarget(std::shared_ptr<IPlayer> player) { m_wpTarget = player; }
	void TriggerAttackHit();
	void SetPosition(const Vector3& pos) {
		GetTransform()->SetPosition(pos);
		if (m_pAIAgent) 
			m_pAIAgent->SetPosition(pos);
	}
	AIPathState GetPathState() const { return m_pAIAgent->GetPathState(); }
	AIBehaviorState GetBehaviorState() const { return m_pAIAgent ? m_pAIAgent->GetBehaviorState() : AIBehaviorState::Idle; }
	const PathDebugInfo& GetPathDebugInfo() const { return m_pAIAgent->GetPathDebugInfo(); }
	float GetMoveSpeedSqXZ() const { return m_fMoveSpeedSqXZ; }

	// 빠른 좀비 (스폰 시 30% 롤 — 오프라인은 로컬 롤, 온라인은 스폰 패킷 zombieType)
	bool IsFast() const { return m_bFast; }
	void SetFast(bool bFast);
	// 이동 속도/확률 — 서버 ZombieManager.h 상수와 동일하게 유지
	static constexpr float ZOMBIE_MOVE_SPEED      = 220.f; // cm/s
	static constexpr float FAST_ZOMBIE_MOVE_SPEED = 450.f; // cm/s
	static constexpr int   FAST_ZOMBIE_PERCENT    = 30;    // 스폰 롤 (%)

	void TakeDamage(float fAmount) { m_fHP = std::max(0.f, m_fHP - fAmount); }
	float GetHP() const { return m_fHP; }
	bool IsDead() const { return m_fHP <= 0.f; }
	bool IsReadyToRemove() const { return m_bReadyToRemove; }

	// 서버 ID (서버에서 할당한 좀비 식별자 — 로컬 AI 전용 좀비는 -1)
	int  GetServerId() const { return m_nServerId; }
	void SetServerId(int id)  { m_nServerId = id; }

	// 서버에서 수신한 상태 적용 (스냅샷 저장 → PostUpdate에서 보간)
	void ApplyServerState(float serverX, float serverZ, float serverYaw,
	                      ZombieBehaviorState state, float receivedTime);

	// 메모리 풀 인터페이스 ─────────────────────────────────────────────────────
	// ZombiePool이 호출. 외부에서 직접 호출 금지.
	bool IsPoolActive() const { return m_bPoolActive; }
	void PoolActivate(); // m_Free → m_Active: 상태 초기화 후 활성화
	void PoolReset();    // m_Active → m_Free: 비활성화 (객체는 재사용 대기)

	Vector3 GetPosition() const;

	virtual void OnBeginCollision(const CollisionResult& collisionResult) override;
	virtual void OnWhileCollision(const CollisionResult& collisionResult) override;
	virtual void OnEndCollision(const CollisionResult& collisionResult) override;
	virtual void OnTraceHit(const RayTraceHitResult& hitResult) override;

private:
	void ApplyGravity();
	void ResolveCollision(Vector3& outv3Delta);
	void UpdateIdleSound();

private:
	std::shared_ptr<IAIAgent> m_pAIAgent;  // AIManagerWrapper가 shared_ptr 반환
	std::weak_ptr<IPlayer> m_wpTarget;   // Scene이 shared_ptr로 소유, Zombie는 참조만

	// 중력
	float m_fVerticalVelocity = 0.f;

	// 충돌 해소용 OBB 목록
	std::vector<BoundingOrientedBox> m_xmOBBCollided;

private:
	const float m_fAcceleration = 10.0_cm;
	const float m_fFriction     = 10.f;
	const float m_fGravity      = -9.8_cm * 10;

	// 감지 범위 — 서버 ZombieManager와 동일 조건 (온/오프라인 일치)
	static constexpr float m_fSightRange      = 10000000.f; // cm — 서버와 동일: 사실상 무한, FOV/LOS 없이 반경 내면 인지
	static constexpr float m_fHearingRange    = 0.0f;       // 서버와 동일 (비활성)

	// FOV (시야각)
	static constexpr float m_fFOVHalfAngleDeg = 60.0f;   // 전방 ±60° → 총 120°
	static constexpr float m_fFOVCosHalf      = 0.5f;    // cos(60°) — dot product 임계값
	static constexpr float m_fCloseRange      = 150.0f;  // 이 거리 이하는 시야각 무시 (공격 거리)
	const float		m_fStepHeight = 50_cm;

	Vector3 m_v3Forward = { 0.f, 0.f, 1.f };  // 좀비 전방 벡터 (매 프레임 이동 방향으로 갱신)

	static constexpr float m_fIdleSoundIntervalMin = 4.f;
	static constexpr float m_fIdleSoundIntervalMax = 8.f;
	static constexpr float m_fIdleSoundPlayChance = 0.3f;
	float m_fIdleSoundTimer = 0.f;

	bool m_bFast = false;

	bool m_bWasVisible = false;  // 이전 프레임 시야 여부 (경보 전파 rising edge 감지용)
	float m_fMoveSpeedSqXZ = 0.f;
	int                m_nServerId = -1;           // 서버 할당 ID (-1 = 로컬 AI 전용)
	ZombieBehaviorState m_eServerBehaviorState = ZBS_Idle; // 최근 수신된 서버 행동 상태 (온라인 애니메이션용)
	float              m_fLastAppliedTime = -1.f;          // 마지막 적용한 서버 상태의 receivedTime (중복 방지)
	float              m_fCurrentYaw      = 0.f;           // 현재 보간 중인 yaw (rad)
	float              m_fTargetYaw       = 0.f;           // 서버에서 수신한 목표 yaw (rad)


	// 서버 위치 스냅샷 기반 보간
	struct ZombieSnapshot {
		Vector3 v3Pos;    // XZ 위치 (Y는 클라이언트 중력)
		float   fYaw;
		float   fTime;    // 수신 시각 (GetNetTimeSec 기준)
	};
	static constexpr size_t MAX_SNAPSHOTS = 8;
	std::deque<ZombieSnapshot> m_Snapshots;
	float m_fInterpDelay = 0.3f;  // 고정: 서버 최대 전송 간격(200ms) * 1.5 — 가변 간격에 자동 조절은 시간축 점프 유발
	float m_fHP = 100.f;
	bool m_bDying = false;
	bool m_bReadyToRemove = false;

	// 풀 상태. true = active(게임플레이 중), false = dormant(재사용 대기)
	// dormant 상태에서는 Update/PostUpdate/Render가 early-return
	bool m_bPoolActive = true;

	// ZombiePool 전용 — 외부에서 직접 접근 금지
	friend class ZombiePool;
	ZombiePool* m_pPool        = nullptr; // 소속 풀 (back-pointer)
	int         m_nActiveIndex = -1;      // m_Active 내 현재 인덱스 (O(1) 제거용)

protected:
	bool TryStepUp(
		const BoundingCapsule& capsule,
		const BoundingOrientedBox& box,
		OUT Vector3& outv3Delta);
	uint32			m_unGroundGraceFrames = 0;
	const uint32	m_unMaxGroundGraceFrames = 4;
};

template<>
struct TraceHitTester<Zombie>
{
	static bool Intersects(Zombie* pObj, Vector3 rayOrigin, Vector3 rayDir, OUT float& outDist)
	{
		if (!pObj) {
			return false;
		}

		auto pCollider = pObj->GetComponent<PlayerCollider>();
		if (!pCollider) {
			return false;
		}

		return pCollider->GetCapsuleWorld().Intersects(rayOrigin, rayDir, outDist);
	}

	static bool Intersects(const std::shared_ptr<Zombie>& pObj, Vector3 rayOrigin, Vector3 rayDir, OUT float& outDist)
	{
		return Intersects(pObj.get(), rayOrigin, rayDir, outDist);
	}

	static constexpr TRACE_HIT_TYPE HitType = TRACE_HIT_TYPE::ZOMBIE;
};
