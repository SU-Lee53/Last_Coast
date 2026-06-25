#include "pch.h"
#include "GameContext.h"
#include "WeaponObject.h"
#include "M4Weapon.h"
#include "AkWeapon.h"
#include "RifleWeapon.h"
#include "PistolWeapon.h"
#include "MeleeWeapon.h"
#include "ShotgunWeapon.h"
#include "LMGWeapon.h"
#include "NodeObject.h"

const std::vector<std::string> GameContext::g_strWeaponNames = {
	"M4",
	"AK",
	"SHOTGUN",
	"RIFLE",
	"LMG",
	"PISTOL",
	"MELEE",
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
	LoadWeaponSounds();
}

void GameContext::LoadWeaponData()
{

	// Load weapon models
	{
		std::string strWeaponFilename[] = {
			"SM_AR4",
			"SM_KA47",
			"SM_Modern_Weapons_Shotgun_01",
			"SM_Modern_Weapons_Sniper_03_No_Scope",
			"SM_Modern_Weapons_LMG_03",
			"SM_Modern_Weapons_Pistol_03",
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

void GameContext::LoadWeaponSounds()
{
	// Rifle1
	SOUND->AddSound("rifle_shot_close", "../Resources/Sounds/Rifle/SW_Weapons_Rifle_Noise-Exterior-Close_01.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("rifle_shot_distant1", "../Resources/Sounds/Rifle/SW_Weapons_Rifle_Noise-Exterior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_on_reload", "../Resources/Sounds/Rifle/SW_Weapons_Rifle_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_mid_reload", "../Resources/Sounds/Rifle/SW_Weapons_Rifle_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_end_reload", "../Resources/Sounds/Rifle/SW_Weapons_Rifle_Bolt_01.wav", false, true, SoundCategory::SFX);
	
	// Shotgun
	SOUND->AddSound("shotgun_shot_close", "../Resources/Sounds/Shotgun/SW_Weapons_Shotgun_Noise-Interior-Close_01.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("shotgun_shot_distant", "../Resources/Sounds/Shotgun/SW_Weapons_Shotgun_Noise-Interior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_on_reload", "../Resources/Sounds/Shotgun/SW_Weapons_Shotgun_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_mid_reload", "../Resources/Sounds/Rifle/SW_Weapons_Shotgun_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_end_reload", "../Resources/Sounds/Shotgun/shotgun_trimmed_longtail.wav", false, true, SoundCategory::SFX);
	
	// Pistol
	SOUND->AddSound("pistol_shot_close", "../Resources/Sounds/Pistol/SW_Weapons_Pistol_Noise-Interior-Close_01.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("pistol_shot_distant", "../Resources/Sounds/Pistol/SW_Weapons_Pistol_Noise-Interior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_on_reload", "../Resources/Sounds/Pistol/SW_Weapons_Pistol_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_mid_reload", "../Resources/Sounds/Rifle/SW_Weapons_Pistol_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_end_reload", "../Resources/Sounds/Pistol/SW_Weapons_Pistol_Slide_01.wav", false, true, SoundCategory::SFX);

	// Impact
	SOUND->AddSound("impact_on_zombie", "../Resources/Sounds/Pistol/SW_ImpactHeadshot_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("impact_on_object", "../Resources/Sounds/Pistol/SW_ImpactPlasterDebris_01.wav", false, true, SoundCategory::SFX);
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
	const auto& pModel = m_pWeaponModels[std::to_underlying(eWeaponType)];
	std::shared_ptr<WeaponObject> pWeapon;
	switch (eWeaponType) {
	case WEAPON_TYPE::M4:      pWeapon = pModel->CopyObject<M4Weapon>();      break;
	case WEAPON_TYPE::AK:      pWeapon = pModel->CopyObject<AkWeapon>();      break;
	case WEAPON_TYPE::RIFLE:   pWeapon = pModel->CopyObject<RifleWeapon>();   break;
	case WEAPON_TYPE::LMG:   pWeapon = pModel->CopyObject<LMGWeapon>();   break;
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

TextureRef<Texture> GameContext::GetImage(const std::string& strName)
{
	auto find = m_pUIImages.find(strName);
	return (find != m_pUIImages.end()) ? find->second : TextureRef<Texture>{};
}
