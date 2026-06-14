#pragma once
#include "WeaponObject.h"

// AK — 단발 hitscan.
class AkWeapon : public WeaponObject {
public:
	AkWeapon() {}
	virtual ~AkWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
