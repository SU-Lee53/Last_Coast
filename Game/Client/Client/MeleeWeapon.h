#pragma once
#include "WeaponObject.h"

// 근접무기 — 총 발사 없음. 근접공격은 별도 경로(ConsumeMelee)로 처리.
class MeleeWeapon : public WeaponObject {
public:
	MeleeWeapon() {}
	virtual ~MeleeWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {}
};
