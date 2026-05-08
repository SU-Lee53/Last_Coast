#pragma once
#include "GameObject.h"

class ModelManager {

	DECLARE_SINGLE(ModelManager)

public:
	void Initialize();
	void LoadGameModels();

public:
	void Add(const std::string& strModelName, std::shared_ptr<IGameObject> pObj);
	std::shared_ptr<IGameObject> Get(const std::string& strObjName);

	std::shared_ptr<IGameObject> LoadOrGet(const std::string& strFileName, bool bUseNameFilenameOnRoot = false);

private:
	std::shared_ptr<IGameObject> LoadModelFromFile(const std::string& strFilePath, bool bUseNamePrefixInKeyOnRoot = false);
	std::shared_ptr<IGameObject> LoadFrameHierarchyFromFile(
		const std::string& strFilename,
		std::shared_ptr<IGameObject> pParent,
		std::shared_ptr<IGameObject> pRoot,
		const nlohmann::json& inJson,
		bool bUseNamePrefixInKeyOnRoot = false,
		int32* outpnIndex = nullptr);

	std::pair<MESHLOADINFO, MATERIALLOADINFO> LoadMeshInfoFromFiles(const nlohmann::json& inJson);

	MATERIALLOADINFO LoadMaterialInfoFromFiles(const nlohmann::json& inJson);
	COLLISIONMESHINFO LoadCollisionInfoFromJson(const nlohmann::json& inJson);
	COLLISIONMESHINFO GatherRenderMeshCollisionInfo(const nlohmann::json& hierarchyJson);

public:
	// 모델 이름으로 콜리전 정보 반환 (없으면 빈 vector)
	const std::vector<COLLISIONMESHINFO>* GetCollisionInfos(const std::string& strModelName) const;

private:
	std::unordered_map<std::string, std::vector<COLLISIONMESHINFO>> m_CollisionInfoPool;

private:
	// Model Pool
	std::unordered_map<std::string, std::shared_ptr<IGameObject>> m_pModelPool;

private:
	inline static std::string g_strModelBasePath = "../Resources/Models";

};
