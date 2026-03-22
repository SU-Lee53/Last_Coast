#pragma once
#include "DynamicObject.h"

class IPlayer;

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
	void SetPosition(const Vector3& pos) {
		GetTransform()->SetPosition(pos);
		if (m_pAIAgent) 
			m_pAIAgent->SetPosition(pos);
	}
	AIPathState GetPathState() const { return m_pAIAgent->GetPathState(); }
	AIBehaviorState GetBehaviorState() const { return m_pAIAgent ? m_pAIAgent->GetBehaviorState() : AIBehaviorState::Idle; }
	const PathDebugInfo& GetPathDebugInfo() const { return m_pAIAgent->GetPathDebugInfo(); }
	float GetMoveSpeedSqXZ() const { return m_fMoveSpeedSqXZ; }

	Vector3 GetPosition() const;

	virtual void OnBeginCollision(const CollisionResult& collisionResult) override;
	virtual void OnWhileCollision(const CollisionResult& collisionResult) override;
	virtual void OnEndCollision(const CollisionResult& collisionResult) override;

private:
	void ApplyGravity();
	void ResolveCollision(Vector3& outv3Delta);

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

	// 감지 범위
	static constexpr float m_fSightRange      = 800.0f;  // 8m
	static constexpr float m_fHearingRange    = 0.0f;    // 3.5m (현재 비활성)

	// FOV (시야각)
	static constexpr float m_fFOVHalfAngleDeg = 60.0f;   // 전방 ±60° → 총 120°
	static constexpr float m_fFOVCosHalf      = 0.5f;    // cos(60°) — dot product 임계값
	static constexpr float m_fCloseRange      = 150.0f;  // 이 거리 이하는 시야각 무시 (공격 거리)

	Vector3 m_v3Forward = { 0.f, 0.f, 1.f };  // 좀비 전방 벡터 (매 프레임 이동 방향으로 갱신)

	bool m_bWasVisible = false;  // 이전 프레임 시야 여부 (경보 전파 rising edge 감지용)
	float m_fMoveSpeedSqXZ = 0.f;
};
