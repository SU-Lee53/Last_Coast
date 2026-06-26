#pragma once
#include "WeaponObject.h"


class ShotgunWeapon : public WeaponObject {
public:
	ShotgunWeapon() {
		m_pFireSound = SOUND->GetSound("shotgun_shot_close");
		m_pBeginReloadSound = SOUND->GetSound("shotgun_on_reload");
		m_pMidReloadSound = SOUND->GetSound("shotgun_mid_reload");
		m_pEndReloadSound = SOUND->GetSound("shotgun_end_reload");
	}
	virtual ~ShotgunWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override;

	void SetPelletCount(int32 nValue) { m_nPelletCount = nValue; }
	void SetSpreadDegree(float fValue) { m_fSpreadDegree = fValue; }

	virtual bool PlayFireSound() override;

private:
	int32 m_nPelletCount  = 8;   
	float m_fSpreadDegree = 4.0f;  
};
