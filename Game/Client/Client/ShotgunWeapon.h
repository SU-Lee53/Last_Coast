#pragma once
#include "WeaponObject.h"


class ShotgunWeapon : public WeaponObject {
public:
	ShotgunWeapon() {}
	virtual ~ShotgunWeapon() {}

	void FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline) override;

	void SetPelletCount(int32 nValue) { m_nPelletCount = nValue; }
	void SetSpreadDegree(float fValue) { m_fSpreadDegree = fValue; }

private:
	int32 m_nPelletCount  = 8;   
	float m_fSpreadDegree = 4.0f;  
};
