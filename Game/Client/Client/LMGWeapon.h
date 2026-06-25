#pragma once
#include "WeaponObject.h"

// LMG — 단발 hitscan.
class LMGWeapon : public WeaponObject {
public:
	LMGWeapon() {
		m_pFireSound = SOUND->GetSound("rifle_shot_close");
		m_pBeginReloadSound = SOUND->GetSound("rifle_on_reload");
		m_pEndReloadSound = SOUND->GetSound("rifle_end_reload");
	}
	virtual ~LMGWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
