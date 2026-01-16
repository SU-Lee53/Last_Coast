#pragma once
#include "Player.h"

class ThirdPersonPlayer : public Player {
public:
	ThirdPersonPlayer();
	virtual ~ThirdPersonPlayer();

public:
	void Initialize() override;
	virtual void ProcessInput() override;
	virtual void Update() override;

	float GetMoveSpeed() const { return m_fMoveSpeed; }
	const Vector3& GetMoveDirection() const { return m_v3MoveDirection; }

	bool IsAiming() const { return m_bAiming; }
	bool IsRunning() const { return m_bRunning; }

private:
	Vector3 m_v3MoveDirection;
	float m_fMoveSpeed = 0.f;
	float m_fMaxMoveSpeed = 1.4_m;
	float m_fAcceleration = 10.0_cm;
	float m_fFriction = 10.f;
	float m_fGraviry = 9.8_km;

	bool m_bAiming = false;
	bool m_bRunning = false;
	bool m_bMouseInUse = false;

	float	m_fMouseSensitivity = 0.1f;



};

