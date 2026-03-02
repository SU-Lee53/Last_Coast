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

std::shared_ptr<IMaterial> MaterialManager::GetMaterialByID(uint64 unID) const
{
	return m_MaterialTable.GetResourceByID(unID);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE MaterialManager::GetCPUHandleByID(uint64 unID) const
{
	return m_MaterialTable.GetCPUHandleByID(unID);
}
