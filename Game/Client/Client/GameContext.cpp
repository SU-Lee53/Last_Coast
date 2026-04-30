#include "pch.h"
#include "GameContext.h"
#include "WeaponObject.h"
#include "NodeObject.h"

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
			m_pWeaponModels[i] = MODEL->LoadOrGet(strWeaponFilename[i], true);
			m_pWeaponModels[i]->SetName(g_cstrWeaponName[i]);
		}
	}

	// Load Weapon Offset data
	{
		std::string strSavePath = "../Resources/Scenes/Weapons.json";
		ifstream in{ strSavePath };
		nlohmann::json jWeaponData = nlohmann::json::parse(in);

		bool checked[_countof(g_cstrWeaponName)] = { false, };

		for (const auto& [k, v] : jWeaponData.items()) {
			for (int i = 0; i < _countof(g_cstrWeaponName); ++i) {
				if (checked[i]) continue;

				if (k == g_cstrWeaponName[i]) {
					m_pWeaponOffsets[i].v3OffsetPosition = ::ReadVector3FromJson(v["OffsetPosition"]);
					m_pWeaponOffsets[i].v3OffsetRotation = ::ReadVector3FromJson(v["OffsetRotation"]);
					checked[i] = true;
					break;
				}
			}
		}
	}

}

std::shared_ptr<IGameObject> GameContext::GetWeaponCopy(WEAPON_TYPE eWeaponType)
{
	return m_pWeaponModels[std::to_underlying(eWeaponType)]->CopyObject<NodeObject>();
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
