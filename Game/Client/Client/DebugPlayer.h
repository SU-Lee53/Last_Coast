#pragma once
#include "Player.h"
class DebugPlayer : public IPlayer {
public:
	DebugPlayer();
	virtual ~DebugPlayer();

private:
	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void Update() override;

private:
	float	m_fPitch = 0.f;
	float	m_fRoll = 0.f;
	float	m_fYaw = 0.f;
	float	m_fMouseSensitivity = 0.001f;
};

