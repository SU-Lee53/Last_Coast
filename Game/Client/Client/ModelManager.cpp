#include "pch.h"
#include "ModelManager.h"

ModelManager::ModelManager(ComPtr<ID3D12Device> pDevice)
{
}

ModelManager::~ModelManager()
{
	OutputDebugStringA("ModelManager Destroy\n");
}

void ModelManager::LoadGameModels()
{
}

void ModelManager::Add(const std::string& strModelName, std::shared_ptr<GameObject> pObj)
{
	if (!m_pModelPool.contains(strModelName)) {
		m_pModelPool.insert({ strModelName, pObj });
	}
}

std::shared_ptr<GameObject> ModelManager::Get(const std::string& strObjName)
{
	auto it = m_pModelPool.find(strObjName);
	if (it == m_pModelPool.end()) {
		return nullptr;
	}

	return it->second;
}

