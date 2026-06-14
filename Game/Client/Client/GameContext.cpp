#include "pch.h"
#include "GameContext.h"
#include "WeaponObject.h"
#include "M4Weapon.h"
#include "AkWeapon.h"
#include "RifleWeapon.h"
#include "PistolWeapon.h"
#include "MeleeWeapon.h"
#include "ShotgunWeapon.h"
#include "NodeObject.h"

const std::vector<std::string> GameContext::g_strWeaponName = {
	"M4",
	"AK",
	"RIFLE",
	"PISTOL",
	"MELEE",
	"SHOTGUN",
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
			"SM_SMG11_Y",          // SHOTGUN placeholder — 모델 추후 교체
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
	const auto& pModel = m_pWeaponModels[std::to_underlying(eWeaponType)];
	std::shared_ptr<WeaponObject> pWeapon;
	switch (eWeaponType) {
	case WEAPON_TYPE::M4:      pWeapon = pModel->CopyObject<M4Weapon>();      break;
	case WEAPON_TYPE::AK:      pWeapon = pModel->CopyObject<AkWeapon>();      break;
	case WEAPON_TYPE::RIFLE:   pWeapon = pModel->CopyObject<RifleWeapon>();   break;
	case WEAPON_TYPE::PISTOL:  pWeapon = pModel->CopyObject<PistolWeapon>();  break;
	case WEAPON_TYPE::MELEE:   pWeapon = pModel->CopyObject<MeleeWeapon>();   break;
	case WEAPON_TYPE::SHOTGUN: pWeapon = pModel->CopyObject<ShotgunWeapon>(); break;
	default:                   pWeapon = pModel->CopyObject<WeaponObject>();  break;
	}

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

std::shared_ptr<IGameObject> GameContext::GeModel(const std::string& strName)
{
	//auto find = m_pGameModels.find(strName);
	//return (find != m_pGameModels.end()) ? find->second : nullptr;
	return nullptr;
}

TextureRef<Texture> GameContext::GetImage(const std::string& strName)
{
	auto find = m_pUIImages.find(strName);
	return (find != m_pUIImages.end()) ? find->second : TextureRef<Texture>{};
}
