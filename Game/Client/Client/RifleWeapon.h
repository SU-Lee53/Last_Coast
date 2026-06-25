#pragma once
#include "WeaponObject.h"

// RIFLE — 단발 hitscan (고데미지 저연사).
class RifleWeapon : public WeaponObject {
public:
	RifleWeapon() {
		m_pFireSound = SOUND->GetSound("rifle_shot_close");
		m_pBeginReloadSound = SOUND->GetSound("rifle_on_reload");
		m_pMidReloadSound = SOUND->GetSound("rifle_mid_reload");
		m_pEndReloadSound = SOUND->GetSound("rifle_end_reload");
	}
	virtual ~RifleWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
