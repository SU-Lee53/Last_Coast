#include "pch.h"
#include "GameContext.h"
#include "WeaponObject.h"
#include "NodeObject.h"

const std::vector<std::string> GameContext::g_strWeaponNames = {
	"M4",
	"AK",
	"RIFLE",
	"PISTOL",
	"MELEE",
	"UNKNOWN",
};

const std::vector<std::string> GameContext::g_strCharacterNames = {
	"player_m_01",
	"player_f_01",
	"player_m_02",
	"player_f_02",
	"UNKNOWN",
};

void GameContext::Initialize()
{
	LoadWeaponData();
	LoadZombieModels();
}

void GameContext::LoadWeaponData()
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
			p->SetName(g_strWeaponNames[i]);
			p->SetWeaponType(static_cast<WEAPON_TYPE>(i));
			m_pWeaponModels[i] = p;
		}
	}

	// Load Weapon Offset data
	{
		std::string strSavePath = "../Resources/Scenes/Weapons.json";
		ifstream in{ strSavePath };
		nlohmann::json jWeaponData = nlohmann::json::parse(in);

		std::vector<bool> checked(g_strWeaponNames.size(), false);

		for (const auto& [k, v] : jWeaponData.items()) {
			for (int i = 0; i < g_strWeaponNames.size(); ++i) {
				if (checked[i]) continue;

				if (k == g_strWeaponNames[i]) {
					m_WeaponStats[i].fDamage = v["Damage"].get<float>();
					m_WeaponStats[i].fFirePerSecond = v["FirePerSecond"].get<float>();
					m_WeaponStats[i].fRecoil = v["Recoil"].get<float>();
					m_WeaponStats[i].fRecoilRecovery = v["RecoilRecovery"].get<float>();
					m_WeaponStats[i].nAmmoPerClip = v["AmmoPerClip"].get<int32>();
					m_WeaponStats[i].v3OffsetPosition = ::ReadVector3FromJson(v["OffsetPosition"]);
					m_WeaponStats[i].v3OffsetRotation = ::ReadVector3FromJson(v["OffsetRotation"]);
					m_WeaponStats[i].v3MuzzlePosition = ::ReadVector3FromJson(v["MuzzlePosition"]);
					checked[i] = true;
					break;
				}
			}
		}
	}
}

void GameContext::LoadPlayerModels()
{
	// Load Zombie Models
	{
		std::string strZombieFilename[] = {
			"player_m_01",
			"player_f_01",
			"player_m_02",
			"player_f_02"
		};

		for (int i = 0; i < g_unZombieModels; ++i) {
			m_pZombieModels[i] = MODEL->LoadOrGet(strZombieFilename[i]);
		}
	}
}

void GameContext::LoadZombieModels()
{
	// Load Zombie Models
	{
		std::string strZombieFilename[] = {
			"Ch10_nonPBR",
			"Yaku J Ignite",
			"Zombiegirl W Kurniawan",
		};

		for (int i = 0; i < g_unZombieModels; ++i) {
			m_pZombieModels[i] = MODEL->LoadOrGet(strZombieFilename[i]);
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
	pWeapon->SetRecoilRecovery(weaponStat.fRecoilRecovery);
	pWeapon->SetAmmoPerClip(weaponStat.nAmmoPerClip);
	pWeapon->SetCurrentAmmoInClip(weaponStat.nAmmoPerClip);
	pWeapon->SetOffsetPosition(weaponStat.v3OffsetPosition);
	pWeapon->SetOffsetRotation(weaponStat.v3OffsetRotation);
	pWeapon->SetMuzzlePositionLocal(weaponStat.v3MuzzlePosition);
	pWeapon->SetWeaponType(eWeaponType);

	return pWeapon;
}

std::shared_ptr<NodeObject> GameContext::GetZombieCopy(uint32 unIndex)
{
	return m_pZombieModels[unIndex]->CopyObject<NodeObject>();
}

TextureRef<Texture> GameContext::GetImage(const std::string& strName)
{
	auto find = m_pUIImages.find(strName);
	return (find != m_pUIImages.end()) ? find->second : TextureRef<Texture>{};
}
