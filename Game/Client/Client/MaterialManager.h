#pragma once
#include "Material.h"

class MaterialManager {

	DECLARE_SINGLE(MaterialManager);

public:
	void Initialize();

	template<typename T> requires std::derived_from<T, IMaterial>
	IMaterial::ID LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo);

	std::shared_ptr<IMaterial> GetMaterialByName(const std::string& strTextureName) const;
	std::shared_ptr<IMaterial> GetMaterialByID(uint64 unID) const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByID(uint64 unID) const;

private:
	MaterialTable m_MaterialTable;

	constexpr static size_t g_unMaxMaterialCount = 300;
};
 
template<typename T> requires std::derived_from<T, IMaterial>
inline IMaterial::ID MaterialManager::LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo)
{
	IMaterial::ID findID = m_MaterialTable.GetID(strNameKey);
	if (findID == MaterialTable::InvalidID) {
		std::shared_ptr<IMaterial> pMaterial = std::make_shared<T>(loadInfo);
		IMaterial::ID id = m_MaterialTable.Register(strNameKey, pMaterial);
		if (id == MaterialTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load material : {}", strNameKey).c_str());
			return MaterialTable::InvalidID;
		}

		return id;
	}

	return findID;
}
