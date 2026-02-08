#include "pch.h"
#include "Player.h"
#include "Camera.h"

IPlayer::IPlayer()
{
}

IPlayer::~IPlayer()
{
}

void IPlayer::PostUpdate()
{
	if (m_pCamera) {
		m_pCamera->Update();
	}

	DynamicObject::PostUpdate();
}
