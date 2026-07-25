#include "pch.h"
#include "Player.h"
#include "Camera.h"

IPlayer::IPlayer()
{
}

IPlayer::~IPlayer()
{
}

void IPlayer::TakeDamage(float fAmount)
{
	if (fAmount <= 0.f || IsDead()) {
		return;
	}

	m_fHP = std::max(0.f, m_fHP - fAmount);
	m_fDamageEffectStrength = 1.f;
}

void IPlayer::RestoreFullHP()
{
	m_fHP = m_fMaxHP;
	m_fDamageEffectStrength = 0.f;
}

void IPlayer::PostUpdate()
{
	m_fDamageEffectStrength = std::max(0.f, m_fDamageEffectStrength - DT * 1.8f);

	if (m_pCamera) {
		m_pCamera->Update();
	}

	DynamicObject::PostUpdate();
}
