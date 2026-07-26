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
	void  Heal(float fAmount) { m_fHP = std::min(m_fMaxHP, m_fHP + fAmount); }   // 오프라인 붕대 회복
	void  SetHP(float fHP)    { m_fHP = std::clamp(fHP, 0.f, m_fMaxHP); }        // 서버 권위 HP 반영 (S2C_PLAYER_HEAL)
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

