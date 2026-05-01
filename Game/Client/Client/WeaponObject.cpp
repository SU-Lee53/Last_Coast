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
	if (ImGui::InputFloat("FirePerSecond", &m_fFirePerSecond)) {
		m_fFireInterval = 60.f / m_fFirePerSecond;
	}
	ImGui::InputFloat("Recoil", &m_fRecoil);
	ImGui::InputFloat("Recoil Recovery", &m_fRecoilRecovery);
	ImGui::InputFloat("ReloadTime", &m_fReloadTime);
	ImGui::DragFloat3("Offset Position", reinterpret_cast<float*>(&m_v3OffsetPosition), 0.1f);
	ImGui::DragFloat3("Offset Rotation", reinterpret_cast<float*>(&m_v3OffsetRotation), 0.1f);
}

void WeaponObject::SaveStat()
{
	std::string strSavePath = "../Resources/Scenes/Weapons.json";
	if (!std::filesystem::exists(strSavePath)) {
		std::filesystem::create_directories(strSavePath);
	}
	std::ifstream in;
	in.open(strSavePath, std::ios::ate);

	nlohmann::json j;
	if (in.tellg() > 0) {
		in.seekg(0, std::ios::beg);
		j = nlohmann::json::parse(in);
	}

	std::string strWeaponName = GameContext::g_strWeaponName[std::to_underlying(m_eWeaponType)];
	auto jStat = j[strWeaponName];

	jStat["Damage"] = m_fDamage;
	jStat["FirePerSecond"] = m_fFirePerSecond;
	jStat["Recoil"] = m_fRecoil;
	jStat["RecoilRecovery"] = m_fRecoilRecovery;
	jStat["ReloadTime"] = m_fReloadTime;

	const Vector3& v3OffsetPos = m_v3OffsetPosition;
	const Vector3& v3OffsetRotation = m_v3OffsetRotation;
	jStat["OffsetPosition"] = { v3OffsetPos.x, v3OffsetPos.y, v3OffsetPos.z };
	jStat["OffsetRotation"] = { v3OffsetRotation.x, v3OffsetRotation.y, v3OffsetRotation.z };

	// Save json for readability 
	std::ofstream out(strSavePath, std::ios::trunc);
	if (!out) {
		__debugbreak();
		return;
	}

	out << j.dump(4) << '\n';
}
