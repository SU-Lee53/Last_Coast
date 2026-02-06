#pragma once
#include "Player.h"

class ThirdPersonPlayer : public IPlayer {
public:
	ThirdPersonPlayer();
	virtual ~ThirdPersonPlayer();

public:
	void Initialize() override;
	virtual void ProcessInput() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

	float GetMoveSpeed() const { return m_fMoveSpeed; }
	float GetMoveSpeedXZ() const;
	float GetMoveSpeedSqXZ() const;
	const Vector3& GetMoveDirection() const { return m_v3MoveDirection; }

	bool IsAiming() const { return m_bAiming; }
	bool IsRunning() const { return m_bRunning; }
	bool IsMouseOn() const { return m_bMouseInUse; }

	virtual void OnBeginCollision(const CollisionResult& collisionResult) override;
	virtual void OnWhileCollision(const CollisionResult& collisionResult) override;
	virtual void OnEndCollision(const CollisionResult& collisionResult) override;

private:
	void ApplyGravity();
	void ResolveCollision(OUT Vector3& outv3Delta);
	bool TryStepUp(
		const BoundingCapsule& capsule, 
		const BoundingOrientedBox& box, 
		OUT Vector3& outv3Delta);

private:
	Vector3	m_v3MoveDirection;
	float	m_fMoveSpeed = 0.f;
	float	m_fVerticalVelocity = 0.f;

	const float	m_fMaxMoveSpeed = 1.4_m;
	const float	m_fAcceleration = 10.0_cm;
	const float	m_fFriction = 10.f;
	const float	m_fGravity = -9.8_cm * 10;

	uint32			m_unGroundGraceFrames = 0;
	const uint32	m_unMaxGroundGraceFrames = 4;
	const float		m_fGroundDeadZoneY = 0.02_cm;
	const float		m_fStepHeight = 30_cm;
	
	const float	m_fMouseSensitivity = 0.1f;

	std::vector<BoundingOrientedBox> m_xmOBBCollided;

	bool m_bMoved = false;
	bool m_bAiming = false;
	bool m_bRunning = false;
	bool m_bMouseInUse = false;

};

