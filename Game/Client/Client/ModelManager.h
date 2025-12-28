#pragma once
#include "GameObject.h"

class ModelManager {
public:
	ModelManager(ComPtr<ID3D12Device> pDevice);
	~ModelManager();

public:
	void LoadGameModels();

public:
	void Add(const std::string& strModelName, std::shared_ptr<GameObject> pObj);
	std::shared_ptr<GameObject> Get(const std::string& strObjName);

private:
	// Texture Pool
	std::unordered_map<std::string, std::shared_ptr<GameObject>> m_pModelPool;

};
