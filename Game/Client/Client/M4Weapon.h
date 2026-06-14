#pragma once
#include "WeaponObject.h"

// M4 — 단발 hitscan.
class M4Weapon : public WeaponObject {
public:
	M4Weapon() {}
	virtual ~M4Weapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
