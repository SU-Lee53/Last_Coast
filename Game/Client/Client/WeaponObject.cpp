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

void WeaponObject::Update()
{
	m_fTimeAfterFire += DT;

	float fYaw = XMConvertToRadians(m_v3OffsetRotation.y);
	float fPitch = XMConvertToRadians(m_v3OffsetRotation.x);
	float fRoll = XMConvertToRadians(m_v3OffsetRotation.z);
	Matrix mtxOffset = Matrix::CreateFromYawPitchRoll(fYaw, fPitch, fRoll);
	mtxOffset.Translation(m_v3OffsetPosition);
	m_mtxOffsetTransform = mtxOffset;
}

bool WeaponObject::TryFire()
{
	if (m_fTimeAfterFire >= m_fFireInterval) {
		m_fTimeAfterFire = 0.f;
		return true;
	}

	return false;
}

const DirectX::SimpleMath::Matrix& WeaponObject::GetOffsetTransform()
{
	return m_mtxOffsetTransform;
}

void WeaponObject::EditStat()
{
	ImGui::InputFloat("Damage", &m_fDamage);
	ImGui::InputFloat("FirePerSecond", &m_fFirePerSecond);
	ImGui::InputFloat("Recoil", &m_fRecoil);
	ImGui::InputFloat("ReloadTime", &m_fReloadTime);
	ImGui::DragFloat3("Offset Position", reinterpret_cast<float*>(&m_v3OffsetPosition), 0.1f);
	ImGui::DragFloat3("Offset Rotation", reinterpret_cast<float*>(&m_v3OffsetRotation), 0.1f);
}
