#pragma once
#include "DynamicObject.h"

class WeaponObject : public DynamicObject {
public:
	WeaponObject() {}
	virtual ~WeaponObject() {}

	void Initialize();

	void ProcessInput() {};
	
	void Update() {};
	
	void SetWeapon(WEAPON_TYPE eWeaponType);

	WEAPON_TYPE GetWeaponType() const { return m_eWeaponType; }

private:
	WEAPON_TYPE m_eWeaponType = WEAPON_TYPE::UNDEFINED;

	Vector3 m_v3LocalMuzzlePosition;

};

