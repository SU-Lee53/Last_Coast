#include "pch.h"
#include "GameContext.h"
#include "WeaponObject.h"
#include "NodeObject.h"

const std::vector<std::string> GameContext::g_strWeaponName = {
	"M4",
	"AK",
	"RIFLE",
	"PISTOL",
	"MELEE"
};

void GameContext::Initialize()
{
	// Load weapon models
	{
		std::string strWeaponFilename[] = {
			"SM_AR4",
			"SM_KA47",
			"old_sks_weapon_model",
			"SM_SMG11_Y",
			"spiked_baseball_bat",
		};

		for (int i = 0; i < g_unWeapons; ++i) {
			//m_pWeaponModels[i] = MODEL->LoadOrGet(strWeaponFilename[i], true);
			auto p = std::make_shared<WeaponObject>();
			p->SetChild(MODEL->LoadOrGet(strWeaponFilename[i], true));
			p->SetName(g_strWeaponName[i]);
			p->SetWeaponType(static_cast<WEAPON_TYPE>(i));
			m_pWeaponModels[i] = p;
		}
	}

	// Load Weapon Offset data
	{
		std::string strSavePath = "../Resources/Scenes/Weapons.json";
		ifstream in{ strSavePath };
		nlohmann::json jWeaponData = nlohmann::json::parse(in);

		std::vector<bool> checked(g_strWeaponName.size(), false);

		for (const auto& [k, v] : jWeaponData.items()) {
			for (int i = 0; i < g_strWeaponName.size(); ++i) {
				if (checked[i]) continue;

				if (k == g_strWeaponName[i]) {
					m_WeaponStats[i].fDamage = v["Damage"].get<float>();
					m_WeaponStats[i].fFirePerSecond = v["FirePerSecond"].get<float>();
					m_WeaponStats[i].fRecoil = v["Recoil"].get<float>();
					m_WeaponStats[i].fReloadTime = v["ReloadTime"].get<float>();
					m_WeaponStats[i].v3OffsetPosition = ::ReadVector3FromJson(v["OffsetPosition"]);
					m_WeaponStats[i].v3OffsetRotation = ::ReadVector3FromJson(v["OffsetRotation"]);
					checked[i] = true;
					break;
				}
			}
		}
	}

}

std::shared_ptr<WeaponObject> GameContext::GetWeaponCopy(WEAPON_TYPE eWeaponType)
{
	auto pWeapon = m_pWeaponModels[std::to_underlying(eWeaponType)]->CopyObject<WeaponObject>();

	const auto& weaponStat = m_WeaponStats[std::to_underlying(eWeaponType)];
	pWeapon->SetDamage(weaponStat.fDamage);
	pWeapon->SetFirePerSecond(weaponStat.fFirePerSecond);
	pWeapon->SetRecoil(weaponStat.fRecoil);
	pWeapon->SetReloadTime(weaponStat.fReloadTime);
	pWeapon->SetOffsetPosition(weaponStat.v3OffsetPosition);
	pWeapon->SetOffsetRotation(weaponStat.v3OffsetRotation);
	pWeapon->SetWeaponType(eWeaponType);

	return pWeapon;
}

std::shared_ptr<IGameObject> GameContext::GeModel(const std::string& strName)
{
	auto find = m_pGameModels.find(strName);
	return (find != m_pGameModels.end()) ? find->second : nullptr;
}

TextureRef<Texture> GameContext::GetImage(const std::string& strName)
{
	auto find = m_pUIImages.find(strName);
	return (find != m_pUIImages.end()) ? find->second : TextureRef<Texture>{};
}
