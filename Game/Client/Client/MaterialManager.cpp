#include "pch.h"
#include "MaterialManager.h"

void MaterialManager::Initialize()
{
	std::lock_guard lock{ m_mtxMaterial };
	m_MaterialTable.Initialize(g_unMaxMaterialCount, false);
}

std::shared_ptr<IMaterial> MaterialManager::GetMaterialByName(const std::string& strTextureName) const
{
	std::lock_guard lock{ m_mtxMaterial };
	return m_MaterialTable.GetResourceByName(strTextureName);
}

std::shared_ptr<IMaterial> MaterialManager::GetMaterialByHandle(const MaterialHandle& handle) const
{
	std::lock_guard lock{ m_mtxMaterial };
	return m_MaterialTable.GetResourceByHandle(handle);
}


