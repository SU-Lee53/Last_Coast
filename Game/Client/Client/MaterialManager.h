#pragma once
#include "Material.h"

class MaterialManager {

	DECLARE_SINGLE(MaterialManager);

public:
	void Initialize();
	void Shutdown();

	template<typename T> requires std::derived_from<T, IMaterial>
	MaterialHandle LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo);

	std::shared_ptr<IMaterial> GetMaterialByName(const std::string& strTextureName) const;
	std::shared_ptr<IMaterial> GetMaterialByHandle(const MaterialHandle& handle) const;
	//CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByID(uint64 unID) const;

private:
	std::shared_ptr<std::mutex> GetMaterialLoadMutex(const std::string& strTextureName);

private:
	MaterialTable m_MaterialTable;
	mutable std::mutex m_mtxMaterial;

	concurrency::concurrent_unordered_map<std::string, std::shared_ptr<std::mutex>> m_MaterialLoadMutexRegistry;

	constexpr static size_t g_unMaxMaterialCount = 2048;
};
 
template<typename T> requires std::derived_from<T, IMaterial>
inline MaterialHandle MaterialManager::LoadMaterial(const std::string& strNameKey, const MATERIALLOADINFO& loadInfo)
{
	{
		std::lock_guard tableLock{ m_mtxMaterial };
		MaterialHandle findHandle = m_MaterialTable.GetHandle(strNameKey);
		if (findHandle.IsValid()) {
			return findHandle;
		}
	}
	
	auto pLoadMutex = GetMaterialLoadMutex(strNameKey);
	std::lock_guard keyLock{ *pLoadMutex };

	{
		std::lock_guard tableLock{ m_mtxMaterial };
		MaterialHandle findHandle = m_MaterialTable.GetHandle(strNameKey);
		if (findHandle.IsValid()) {
			return findHandle;
		}
	}

	std::shared_ptr<IMaterial> pMaterial = std::make_shared<T>();
	pMaterial->Initialize(loadInfo);

	MaterialHandle handle;
	{
		std::lock_guard tableLock{ m_mtxMaterial };
		handle = m_MaterialTable.Register(strNameKey, pMaterial);
	}

	if (!handle.IsValid()) {
		OutputDebugStringA(std::format("Failed to load material : {}\n", strNameKey).c_str());
	}

	return handle;

}
