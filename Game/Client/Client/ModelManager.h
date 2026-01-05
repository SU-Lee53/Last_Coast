#pragma once
#include "GameObject.h"

class ModelManager {

	DECLARE_SINGLE(ModelManager)

public:
	void Initialize();
	void LoadGameModels();

public:
	void Add(const std::string& strModelName, std::shared_ptr<GameObject> pObj);
	std::shared_ptr<GameObject> Get(const std::string& strObjName);

	std::shared_ptr<GameObject> LoadOrGet(const std::string& strFileName);

private:
	std::shared_ptr<GameObject> LoadFrameHierarchyFromFile(std::shared_ptr<GameObject> pParent, const nlohmann::json& inJson);
	std::shared_ptr<GameObject> LoadModelFromFile(const std::string& strFilePath);

	std::pair<MESHLOADINFO, MATERIALLOADINFO> LoadMeshInfoFromFiles(const nlohmann::json& inJson);
	MATERIALLOADINFO LoadMaterialInfoFromFiles(const nlohmann::json& inJson);

private:
	// Model Pool
	std::unordered_map<std::string, std::shared_ptr<GameObject>> m_pModelPool;

private:
	inline static std::string g_strModelBasePath = "../Resources/Models";

};
