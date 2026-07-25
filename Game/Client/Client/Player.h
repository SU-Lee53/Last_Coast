#pragma once
#include "DynamicObject.h"

class Camera;

interface IPlayer abstract : public DynamicObject {
public:
	IPlayer();
	virtual ~IPlayer();

	virtual void PostUpdate() override;
	virtual bool IsCharacter() const override { return true; }  // 탈출 컷씬 숨김 대상

public:
	const std::shared_ptr<Camera>& GetCamera() const { return m_pCamera; };

	virtual void TakeDamage(float fAmount);
	float GetHP()    const { return m_fHP; }
	float GetMaxHP() const { return m_fMaxHP; }
	float GetDamageEffectStrength() const { return m_fDamageEffectStrength; }
	bool  IsDead()   const { return m_fHP <= 0.f; }
	void  RestoreFullHP();

protected:
	std::shared_ptr<Camera> m_pCamera = nullptr;

	float m_fHP    = 100.f;
	float m_fMaxHP = 100.f;
	float m_fDamageEffectStrength = 0.f;

};

