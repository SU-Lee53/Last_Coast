#include "pch.h"
#include "WeaponObject.h"
#include "MuzzleFlashEffect.h"

#include "StaticObject.h"
#include "Zombie.h"

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
	if (m_bInReload) return false;
	if (m_nAmmoInClip <= 0) return false;

	if (m_fTimeAfterFire >= m_fFireInterval) {
		--m_nAmmoInClip;
		m_fTimeAfterFire = 0.f;

		const auto& pCamera = CUR_SCENE->GetCamera();
		Vector3 v3CamPos = pCamera->GetPosition();
		Vector3 v3CamDir = pCamera->GetLook();

		bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();

		if (bOnline) {
			// 온라인: 서버에 사격 요청만 전송. 판정은 서버가 처리.
			NETWORK->SendPlayerShoot(v3CamPos, v3CamDir, m_v3MuzzlePositionWorld);
		}
		else {
			// 오프라인: 기존 로컬 레이트레이스 판정
			RayTraceDesc rayDesc{};
			rayDesc.v3Origin = v3CamPos;
			rayDesc.v3Direction = v3CamDir;
			rayDesc.fMaxDistance = 5000.f;
			rayDesc.fDamage = m_fDamage;
			rayDesc.pInstigator = m_wpOwner.lock().get();
			rayDesc.pSourceObject = this;

			RayTraceHitResult hit{};
			CUR_SCENE->GetWorld().LineTraceSingle<StaticObject, Zombie>(rayDesc, hit);
		}

		// 총구 이펙트는 항상 즉시 출력
		ParticleEffectSpawnDesc particleDesc;
		{
			particleDesc.v3Position = m_v3MuzzlePositionWorld;
			particleDesc.v3Direction = GetTransform()->GetLook();
			particleDesc.v3Normal = GetTransform()->GetUp();
			particleDesc.mtxWorld = Matrix::CreateWorld(particleDesc.v3Position, particleDesc.v3Direction, particleDesc.v3Normal);
		};

		PARTICLE->Spawn<MuzzleFlashEffect>(particleDesc);
		return true;
	}

	return false;
}

bool WeaponObject::BeginReload()
{
	if (m_bInReload) return false;

	m_bInReload = true;
	m_fReloadTimer = 0.f;
	return true;
}

bool WeaponObject::EndReload()
{
	if (!m_bInReload) return false;

	m_nTotalAmmo -= 30 - m_nAmmoInClip;
	m_nAmmoInClip = 30;
	m_bInReload = false;
	return true;
}

void WeaponObject::UpdateMuzzlePositionWorld(const Matrix& mtxWorld)
{
	m_v3MuzzlePositionWorld = Vector3::Transform(m_v3MuzzlePositionLocal, mtxWorld);
}

const Matrix& WeaponObject::GetOffsetTransform()
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
	ImGui::InputInt("AmmoPerClip", &m_nAmmoPerClip);
	ImGui::DragFloat3("Offset Position", reinterpret_cast<float*>(&m_v3OffsetPosition), 0.1f);
	ImGui::DragFloat3("Offset Rotation", reinterpret_cast<float*>(&m_v3OffsetRotation), 0.1f);
	ImGui::DragFloat3("Muzzel Local Position", reinterpret_cast<float*>(&m_v3MuzzlePositionLocal), 0.1f);
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

	std::string strWeaponName = GameContext::g_strWeaponNames[std::to_underlying(m_eWeaponType)];
	auto& jStat = j[strWeaponName];

	jStat["Damage"] = m_fDamage;
	jStat["FirePerSecond"] = m_fFirePerSecond;
	jStat["Recoil"] = m_fRecoil;
	jStat["RecoilRecovery"] = m_fRecoilRecovery;
	jStat["AmmoPerClip"] = m_nAmmoPerClip;

	const Vector3& v3MuzzlePos = m_v3MuzzlePositionLocal;
	const Vector3& v3OffsetPos = m_v3OffsetPosition;
	const Vector3& v3OffsetRotation = m_v3OffsetRotation;
	jStat["MuzzlePosition"] = { v3MuzzlePos.x, v3MuzzlePos.y, v3MuzzlePos.z };
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
