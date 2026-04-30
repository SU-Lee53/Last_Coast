#include "pch.h"
#include "WeaponObject.h"

void WeaponObject::Initialize() 
{
	for (auto& component : m_pComponents) {
		if (!GetComponent<Transform>()) {
			AddComponent<Transform>();
		}

		if (component) {
			component->Initialize();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	if (m_pParent.expired() && !GetComponent<ICollider>()) {
		AddComponent<DynamicCollider>();
		GetComponent<DynamicCollider>()->Initialize();
	}

	m_bInitialized = true;
};

void WeaponObject::SetWeapon(WEAPON_TYPE eWeaponType)
{
	m_eWeaponType = eWeaponType;

	// Add child if children is empty
	if (m_pChildren.size() == 0) {
		auto pWeapon = GCTX->GetWeaponCopy(eWeaponType);
		SetChild(pWeapon);
		return;
	}

	// Swap if children is not empty
	auto pWeapon = GCTX->GetWeaponCopy(eWeaponType);
	SwapChild(0, pWeapon);
}
