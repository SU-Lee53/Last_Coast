#include "pch.h"
#include "MaterialManager.h"

void MaterialManager::Initialize()
{
	m_MaterialTable.Initialize(g_unMaxMaterialCount, false);
}

std::shared_ptr<IMaterial> MaterialManager::GetMaterialByName(const std::string& strTextureName) const
{
	return m_MaterialTable.GetResourceByName(strTextureName);
}

std::shared_ptr<IMaterial> MaterialManager::GetMaterialByHandle(const MaterialHandle& handle) const
{
	return m_MaterialTable.GetResourceByHandle(handle);
}


