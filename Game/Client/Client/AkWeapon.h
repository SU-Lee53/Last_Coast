#pragma once
#include "WeaponObject.h"

// AK — 단발 hitscan.
class AkWeapon : public WeaponObject {
public:
	AkWeapon() {
		m_pFireSound = SOUND->GetSound("ak_shot_close");
		m_pBeginReloadSound = SOUND->GetSound("rifle_on_reload");
		m_pMidReloadSound = SOUND->GetSound("rifle_mid_reload");
		m_pEndReloadSound = SOUND->GetSound("rifle_end_reload");
	}
	virtual ~AkWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
