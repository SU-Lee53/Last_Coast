#pragma once
#include "Player.h"

class ThirdPersonPlayer : public Player {
public:
	ThirdPersonPlayer();
	virtual ~ThirdPersonPlayer();

private:
	void Initialize() override;
	virtual void ProcessInput() override;

private:
	float	m_fPitch = 0.f;
	float	m_fRoll = 0.f;
	float	m_fYaw = 0.f;
	float	m_fMouseSensitivity = 0.001f;



};

