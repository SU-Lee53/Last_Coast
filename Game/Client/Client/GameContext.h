#pragma once

/// - 게임에 사용될 리소스들을 주로 관리
///		- UI 용 이미지
///		- 무기 모델
///		- 기타 상수
///		+ 사운드 (06.25)

class WeaponObject;
class NodeObject;

class GameContext {
public:
	// Constants, Enums
	constexpr static uint32 g_unWeapons = std::to_underlying(WEAPON_TYPE::COUNT);
	const static  std::vector<std::string> g_strWeaponNames;
	const static  std::vector<std::string> g_strCharacterNames;

	constexpr static uint32 g_unZombieModels = 3;
	constexpr static uint32 g_unCharacterModels = 4;

	struct PlayerData {
		int32 m_nPlayerID;
		std::string m_nPlayerName;
	};

	struct GameData {
		int32 m_nCurModelIndex = 0;
		int32 m_nWeapon1Index = 0;	// 1번 슬롯 주무기
		int32 m_nWeapon2Index = 1;	// 2번 슬롯 주무기 (3번 슬롯은 PISTOL 고정)
	};

public:
	void Initialize();

	void LoadWeaponData();
	void LoadGameSounds();
	void LoadPlayerModels();
	void LoadZombieModels();
	void LoadHelicopterModel();

	// Getters
	std::shared_ptr<WeaponObject> GetWeaponCopy(WEAPON_TYPE eWeaponType);
	std::shared_ptr<NodeObject> GetZombieCopy(uint32 unIndex);
	TextureRef<Texture> GetImage(const std::string& strName);

	const Vector3& GetWeaponOffsetPosition(WEAPON_TYPE eWeaponType) const { return m_WeaponStats[std::to_underlying(eWeaponType)].v3OffsetPosition; }
	const Vector3& GetWeaponOffsetRotation(WEAPON_TYPE eWeaponType) const { return m_WeaponStats[std::to_underlying(eWeaponType)].v3OffsetRotation; }

	void SetPlayerData(const PlayerData& playerData) { m_PlayerData = playerData; }
	void SetGameData(const GameData& gameData) { m_GameData = gameData; }

	const PlayerData& GetPlayerData() const { return m_PlayerData; }
	const GameData& GetGameData() const { return m_GameData; }

private:
	// Game Context Data
	PlayerData m_PlayerData;
	GameData m_GameData;

	// Game Resoruce Containers
	std::array<std::shared_ptr<IGameObject>, g_unWeapons> m_pWeaponModels;
	std::array<std::shared_ptr<IGameObject>, g_unZombieModels> m_pZombieModels;
	std::array<std::shared_ptr<IGameObject>, g_unCharacterModels> m_pCharacterModels;
	//std::unordered_map<std::string, std::shared_ptr<IGameObject>> m_pGameModels;
	std::unordered_map<std::string, TextureRef<Texture>> m_pUIImages;

	struct WeaponStats {
		Vector3 v3OffsetPosition;
		Vector3 v3OffsetRotation;
		Vector3 v3MuzzlePosition;
		float fDamage;
		float fFirePerSecond;
		float fRecoil;
		float fRecoilRecovery;
		int32 nAmmoPerClip;
	};

	std::array<WeaponStats, g_unWeapons> m_WeaponStats;
	std::shared_ptr<IGameObject> m_pHelicopterModel = nullptr;

};

