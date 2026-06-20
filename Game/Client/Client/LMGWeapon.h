#pragma once
#include "WeaponObject.h"

// LMG — 단발 hitscan.
class LMGWeapon : public WeaponObject {
public:
	LMGWeapon() {}
	virtual ~LMGWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
