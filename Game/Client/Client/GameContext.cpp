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
	LoadGameSounds();
	LoadHelicopterModel();
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

void GameContext::LoadGameSounds()
{
	const std::string strSoundPath = "../Resources/Sounds/";

	// Rifle1
	SOUND->AddSound("m4_shot_close", strSoundPath + "Rifle/SW_Weapons_Rifle_Noise-Exterior-Close_01.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("rifle_shot_distant1", strSoundPath + "Rifle/SW_Weapons_Rifle_Noise-Exterior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_on_reload", strSoundPath + "Rifle/SW_Weapons_Rifle_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_mid_reload", strSoundPath + "Rifle/SW_Weapons_Rifle_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_end_reload", strSoundPath + "Rifle/SW_Weapons_Rifle_Bolt_01.wav", false, true, SoundCategory::SFX);
	
	SOUND->AddSound("ak_shot_close", strSoundPath + "Rifle/ak_fire.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("lmg_shot_close", strSoundPath + "Rifle/lmg_fire.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("rifle_shot_close", strSoundPath + "Rifle/rifle_fire.wav", false, true, SoundCategory::SFX);



	// Shotgun
	SOUND->AddSound("shotgun_shot_close", strSoundPath + "Shotgun/shotgun_fire.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("shotgun_shot_distant", strSoundPath + "Shotgun/SW_Weapons_Shotgun_Noise-Interior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_on_reload", strSoundPath + "Shotgun/SW_Weapons_Shotgun_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_mid_reload", strSoundPath + "Shotgun/SW_Weapons_Shotgun_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("shotgun_end_reload", strSoundPath + "Shotgun/shotgun_trimmed_longtail.wav", false, true, SoundCategory::SFX);
	
	// Pistol
	SOUND->AddSound("pistol_shot_close", strSoundPath + "Pistol/SW_Weapons_Pistol_Noise-Interior-Close_01.wav", false, true, SoundCategory::SFX);
	//SOUND->AddSound("pistol_shot_distant", strSoundPath + "Pistol/SW_Weapons_Pistol_Noise-Interior-Distant_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_on_reload", strSoundPath + "Pistol/SW_Weapons_Pistol_ClipIn_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_mid_reload", strSoundPath + "Pistol/SW_Weapons_Pistol_ClipOut_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("pistol_end_reload", strSoundPath + "Pistol/SW_Weapons_Pistol_Slide_01.wav", false, true, SoundCategory::SFX);

	// Impact
	SOUND->AddSound("impact_on_zombie", strSoundPath + "Impacts/SW_ImpactHeadshot_01.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("impact_on_object", strSoundPath + "Impacts/SW_ImpactPlasterDebris_01.wav", false, true, SoundCategory::SFX);

	// Impact
	// 폭발은 큰 소리 — min을 넓게 잡아 15m까지 풀볼륨, 이후 완만히 감쇠 (기본 100cm면 20m서 5% 볼륨)
	SOUND->AddSound("explosion", strSoundPath + "Explosive.wav", false, true, SoundCategory::SFX, 15_m, 300_m);
	SOUND->AddSound("helicopter_crash", strSoundPath + "Helicopter/helicopter_crash.wav", false, true, SoundCategory::SFX, 100_m, 500_m);
	SOUND->AddSound("helicopter_explosion", strSoundPath + "Helicopter/helicopter_explosion.wav", false, true, SoundCategory::SFX, 150_m, 500_m);
	SOUND->AddSound("helicopter_landing", strSoundPath + "Helicopter/helicopter_landing.wav", false, true, SoundCategory::SFX, 100_m, 500_m);
	SOUND->AddSound("helicopter_takeoff", strSoundPath + "Helicopter/helicopter_takeoff.wav", false, true, SoundCategory::SFX, 100_m, 500_m);
	SOUND->AddSound("helicopter_idle", strSoundPath + "Helicopter/helicopter_idle_loop.wav", true, true, SoundCategory::SFX, 100_m, 500_m);

	SOUND->AddSound("ambience_night", strSoundPath + "Ambience/Malibu_Night_Loop.wav", true, false, SoundCategory::BGM);
	SOUND->AddSound("ambience_foggy_dawn", strSoundPath + "Ambience/Malibu_FoggyDawn_Loop.wav", true, false, SoundCategory::BGM);
	SOUND->AddSound("ambience_sunrise", strSoundPath + "Ambience/Malibu_Sunrise_Loop.wav", true, false, SoundCategory::BGM);

	SOUND->AddSound("footstep_left", strSoundPath + "Footstep_Left.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("footstep_right", strSoundPath + "Footstep_Right.wav", false, true, SoundCategory::SFX);

	SOUND->AddSound("zombie_idle_1", strSoundPath + "Zombie/idle1.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_2", strSoundPath + "Zombie/idle2.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_3", strSoundPath + "Zombie/idle3.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_4", strSoundPath + "Zombie/idle4.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_5", strSoundPath + "Zombie/idle5.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_6", strSoundPath + "Zombie/idle6.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_7", strSoundPath + "Zombie/idle7.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_8", strSoundPath + "Zombie/idle8.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_9", strSoundPath + "Zombie/idle9.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_10", strSoundPath + "Zombie/idle10.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_11", strSoundPath + "Zombie/idle11.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_12", strSoundPath + "Zombie/idle12.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_13", strSoundPath + "Zombie/idle13.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_14", strSoundPath + "Zombie/idle14.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_idle_15", strSoundPath + "Zombie/idle15.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_1", strSoundPath + "Zombie/attack1.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_2", strSoundPath + "Zombie/attack2.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_3", strSoundPath + "Zombie/attack3.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_4", strSoundPath + "Zombie/attack4.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_5", strSoundPath + "Zombie/attack5.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_6", strSoundPath + "Zombie/attack6.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_7", strSoundPath + "Zombie/attack7.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_8", strSoundPath + "Zombie/attack8.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_9", strSoundPath + "Zombie/attack9.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_10", strSoundPath + "Zombie/attack10.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_11", strSoundPath + "Zombie/attack11.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_attack_12", strSoundPath + "Zombie/attack12.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_1", strSoundPath + "Zombie/dead1.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_2", strSoundPath + "Zombie/dead2.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_3", strSoundPath + "Zombie/dead3.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_4", strSoundPath + "Zombie/dead4.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_5", strSoundPath + "Zombie/dead5.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_6", strSoundPath + "Zombie/dead6.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_7", strSoundPath + "Zombie/dead7.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_8", strSoundPath + "Zombie/dead8.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_9", strSoundPath + "Zombie/dead9.wav", false, true, SoundCategory::SFX);
	SOUND->AddSound("zombie_dead_10", strSoundPath + "Zombie/dead10.wav", false, true, SoundCategory::SFX);

}

void GameContext::LoadPlayerModels()
{
	// Load Player Models
	{
		std::string strPlayerFilename[] = {
			"player_m_01",
			"player_f_01",
			"player_m_02",
			"player_f_02"
		};

		for (int i = 0; i < g_unZombieModels; ++i) {
			m_pCharacterModels[i] = MODEL->LoadOrGet(strPlayerFilename[i]);
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

void GameContext::LoadHelicopterModel()
{
	m_pHelicopterModel = MODEL->LoadOrGet("Gunship")->CopyObject<NodeObject>();
}

std::string GameContext::GetWeaponIconPath(WEAPON_TYPE eWeaponType)
{
	std::string strName = g_strWeaponNames[std::to_underlying(eWeaponType)];
	std::transform(strName.begin(), strName.end(), strName.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return std::format("../Resources/Textures/{}.png", strName);
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
