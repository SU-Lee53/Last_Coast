#pragma once
#include "Material.h"

class MaterialManager {

	DECLARE_SINGLE(MaterialManager);

public:
	void Initialize();

	template<typename T> requires std::derived_from<T, IMaterial>
	MaterialHandle LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo);

	std::shared_ptr<IMaterial> GetMaterialByName(const std::string& strTextureName) const;
	std::shared_ptr<IMaterial> GetMaterialByHandle(const MaterialHandle& handle) const;
	//CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByID(uint64 unID) const;

private:
	MaterialTable m_MaterialTable;

	constexpr static size_t g_unMaxMaterialCount = 300;
};
 
template<typename T> requires std::derived_from<T, IMaterial>
inline MaterialHandle MaterialManager::LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo)
{
	MaterialHandle findHandle = m_MaterialTable.GetHandle(strNameKey);
	if (!findHandle.IsValid()) {
		std::shared_ptr<IMaterial> pMaterial = std::make_shared<T>();
		MaterialHandle handle = m_MaterialTable.Register(strNameKey, pMaterial);
		if (!handle.IsValid()) {
			OutputDebugStringA(std::format("Failed to load material : {}\n", strNameKey).c_str());
		}
		else {
			pMaterial->Initialize(loadInfo);
		}

		return handle;
	}

	return findHandle;
}
