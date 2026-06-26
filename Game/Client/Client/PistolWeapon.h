#pragma once
#include "WeaponObject.h"

// 권총 — 단발 hitscan (반자동).
class PistolWeapon : public WeaponObject {
public:
	PistolWeapon() {
		m_pFireSound = SOUND->GetSound("pistol_shot_close");
		m_pBeginReloadSound = SOUND->GetSound("pistol_on_reload");
		m_pMidReloadSound = SOUND->GetSound("pistol_mid_reload");
		m_pEndReloadSound = SOUND->GetSound("pistol_end_reload");
	}
	virtual ~PistolWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override {
		FireSingleHitscan(v3CamPos, v3CamDir, bOnline);
	}
};
