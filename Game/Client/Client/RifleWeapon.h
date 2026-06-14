#pragma once
#include "WeaponObject.h"

// RIFLE — 단발 hitscan (고데미지 저연사).
class RifleWeapon : public WeaponObject {
public:
	RifleWeapon() {}
	virtual ~RifleWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
