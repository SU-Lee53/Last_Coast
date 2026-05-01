#pragma once

/// - 게임에 사용될 리소스들을 주로 관리
///		- UI 용 이미지
///		- 무기 모델
///		- 기타 상수

class WeaponObject;

class GameContext {
public:
	// Constants, Enums
	constexpr static uint32 g_unWeapons = std::to_underlying(WEAPON_TYPE::COUNT);
	const static std::string g_strWeaponName[g_unWeapons];

public:
	void Initialize();

	// Getters
	std::shared_ptr<WeaponObject> GetWeaponCopy(WEAPON_TYPE eWeaponType);
	std::shared_ptr<IGameObject> GeModel(const std::string& strName);
	TextureRef<Texture> GetImage(const std::string& strName);


	const Vector3& GetWeaponOffsetPosition(WEAPON_TYPE eWeaponType) const { return m_WeaponStats[std::to_underlying(eWeaponType)].v3OffsetPosition; }
	const Vector3& GetWeaponOffsetRotation(WEAPON_TYPE eWeaponType) const { return m_WeaponStats[std::to_underlying(eWeaponType)].v3OffsetRotation; }

private:
	// Containers
	std::array<std::shared_ptr<IGameObject>, g_unWeapons> m_pWeaponModels;
	std::unordered_map<std::string, std::shared_ptr<IGameObject>> m_pGameModels;
	std::unordered_map<std::string, TextureRef<Texture>> m_pUIImages;

	struct WeaponStats {
		Vector3 v3OffsetPosition;
		Vector3 v3OffsetRotation;
		float fDamage;
		float fFirePerSecond;
		float fRecoil;
		float fReloadTime;
	};

	std::array<WeaponStats, g_unWeapons> m_WeaponStats;

};

