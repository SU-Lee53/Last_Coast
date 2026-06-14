#pragma once
#include "WeaponObject.h"

// 권총 — 단발 hitscan (반자동).
class PistolWeapon : public WeaponObject {
public:
	PistolWeapon() {}
	virtual ~PistolWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
