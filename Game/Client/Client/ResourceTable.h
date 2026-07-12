#pragma once

class Texture;
struct IMaterial;


template <typename KeyType, typename ResourceType>
struct ResourceEntry {
	using ResourcePtr = std::shared_ptr<ResourceType>;

	ResourcePtr pResource;
	int32 nRefCount = 0;
	bool bAlive = 0;

	KeyType key{};
};

template<typename KeyType, typename ResourceType, typename Hasher = typename std::hash<KeyType>>
class ResourceTable;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceHandle

template<typename KeyType, typename ResourceType, typename Hasher = typename std::hash<KeyType>>
class ResourceHandle {
public:
	using TableType = ResourceTable<KeyType, ResourceType, Hasher>;
	using ID = typename TableType::ID;
	using ResourcePtr = typename TableType::ResourcePtr;

public:
	ResourceHandle() = default;

	ResourceHandle(TableType* pResourceTable, ID id) : m_pResourceTable(pResourceTable), m_id(id) {
		if (m_pResourceTable != nullptr && m_id != TableType::InvalidID) {
			m_pResourceTable->AddRef(m_id);
		}
	}

	ResourceHandle(const ResourceHandle& other) : m_pResourceTable(other.m_pResourceTable), m_id(other.m_id) {
		if (m_pResourceTable != nullptr && m_id != TableType::InvalidID) {
			m_pResourceTable->AddRef(m_id);
		}
	}

	ResourceHandle(ResourceHandle&& other) noexcept : m_pResourceTable(other.m_pResourceTable), m_id(other.m_id) {
		other.m_pResourceTable = nullptr;
		other.m_id = TableType::InvalidID;
	}

	~ResourceHandle() {
		Reset();
	}

	ResourceHandle& operator=(const ResourceHandle& other) {
		if (this == &other) {
			return *this;
		}

		Reset();

		m_pResourceTable = other.m_pResourceTable;
		m_id = other.m_id;

		if (m_pResourceTable != nullptr && m_id != TableType::InvalidID) {
			m_pResourceTable->AddRef(m_id);
		}

		return *this;
	}

	ResourceHandle& operator=(ResourceHandle&& other) noexcept {
		if (this == &other) {
			return *this;
		}

		Reset();

		m_pResourceTable = other.m_pResourceTable;
		m_id = other.m_id;

		other.m_pResourceTable = nullptr;
		other.m_id = TableType::InvalidID;

		return *this;
	}

	void Reset() {
		if (m_pResourceTable != nullptr && m_id != TableType::InvalidID) {
			m_pResourceTable->Release(m_id);
		}

		m_pResourceTable = nullptr;
		m_id = TableType::InvalidID;
	}

	ID GetID() const {
		return m_id;
	}

	ResourcePtr GetResource() const {
		if (!IsValid()) {
			return nullptr;
		}

		return m_pResourceTable->GetResourceByID(m_id);
	}

	bool IsValid() const {
		return m_pResourceTable != nullptr 
			&& m_id != TableType::InvalidID
			&& m_pResourceTable->IsAlive(m_id);
	}

private:
	TableType* m_pResourceTable = nullptr;
	ID m_id = TableType::InvalidID;
};

using MaterialHandle = ResourceHandle<std::string, IMaterial>;
using TextureHandle = ResourceHandle<std::string, Texture>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceTable

template<typename KeyType, typename ResourceType, typename Hasher>
class ResourceTable {
public:
	friend class ResourceHandle<KeyType, ResourceType, Hasher>;

	using ResourcePtr = typename ResourceEntry<KeyType, ResourceType>::ResourcePtr;
	using ID = uint64_t;
	using Handle = ResourceHandle<KeyType, ResourceType, Hasher>;
	constexpr static ID InvalidID = INVALID_ID;

	using CleanUpFn = std::function<void(const ResourcePtr&)>;
	void SetCleanUpCallback(CleanUpFn fn) {
		std::lock_guard lock{ m_mtxTable };
		m_CleanUpFn = std::move(fn);
	}



public:
	// Initialize
	void Initialize(size_t nMaxSize, bool bUseDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, D3D12_DESCRIPTOR_HEAP_FLAGS d3dHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
		std::lock_guard lock{ m_mtxTable };
		m_unMaxSize = nMaxSize;
		m_ResourceEntries.resize(nMaxSize);
		m_KeyIDMap.reserve(nMaxSize);
	}

	// Register
	Handle Register(const KeyType& key, ResourcePtr pResource) {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it != m_KeyIDMap.end()) {
			return { this, it->second };
		}

		ID id = AllocateID();
		if (id == INVALID_ID) {
			return {};
		}

		auto& entry = m_ResourceEntries[id];
		entry.pResource = pResource;
		entry.nRefCount = 0;
		entry.bAlive = true;
		entry.key = key;

		m_KeyIDMap.emplace(key, id);
		return { this, id };
	}
	
	// Look Up
	Handle GetHandle(const KeyType& key) {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end()) {
			return {};
		}

		return { this, it->second };
	}
	
	ResourcePtr GetResourceByID(ID id) const {
		if (id >= m_ResourceEntries.size()) {
			return nullptr;
		}

		if (m_ResourceEntries[id].bAlive == false) {
			return nullptr;
		}

		return m_ResourceEntries[id].pResource;
	}
	
	ResourcePtr GetResourceByHandle(const Handle& handle) const {
		return handle.GetResource();
	}

	ResourcePtr GetResourceByName(const KeyType& key) const {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end())
			return nullptr;

		ID id = it->second;
		if (m_ResourceEntries[id].bAlive == false) {
			return nullptr;
		}

		return m_ResourceEntries[id].pResource;
	}

	const std::vector<ResourceEntry<KeyType, ResourceType>>& GetEntries() const {
		return m_ResourceEntries;
	}

private:
	bool AddRef(ID id) {
		if (id >= m_unMaxSize || id >= m_ResourceEntries.size()) {
			return false;
		}

		auto& entry = m_ResourceEntries[id];
		std::atomic_ref<int32> refCount{ entry.nRefCount };

		int32 current = refCount.load();

		while (current > 0) {
			if (refCount.compare_exchange_weak(current, current + 1)) {
				return true;
			}
		}

		std::lock_guard lock{ m_mtxTable };
		if (!entry.bAlive) {
			return false;
		}

		refCount.fetch_add(1);
		return true;
	}

	bool Release(ID id) {
		if (id >= m_unMaxSize || id >= m_ResourceEntries.size()) {
			return false;
		}

		auto& entry = m_ResourceEntries[id];
		std::atomic_ref<int32> refCount{ entry.nRefCount };

		int32 current = refCount.load();

		while (current > 1) {
			if (refCount.compare_exchange_weak(current, current - 1)) {
				return true;
			}
		}

		std::lock_guard lock{ m_mtxTable };
		if (!entry.bAlive) {
			return false;
		}

		const int32 previous = refCount.fetch_sub(1);
		if (previous > 1) {
			return true;
		}

		if (previous != 1) {
			refCount.fetch_add(1);
			assert(false && "Invalud resource ref count");
			return false;
		}

		if (m_CleanUpFn && entry.pResource) {
			m_CleanUpFn(entry.pResource);
		}

		m_KeyIDMap.erase(entry.key);
		entry.bAlive = false;
		entry.key = KeyType{};
		FreeID(id);

		return true;
	}

	ID AllocateID() {
		if (!m_FreeIDs.empty()) {
			ID id = m_FreeIDs.back();
			m_FreeIDs.pop_back();
			return id;
		}

		if (m_NextID >= m_unMaxSize) {
			return InvalidID;
		}

		return m_NextID++;
	}

	void FreeID(ID id) {
		m_FreeIDs.push_back(id);
	}

	bool IsAlive(ID id) const {
		if (id >= m_unMaxSize) {
			return false;
		}
		return m_ResourceEntries[id].bAlive;
	}

private:
	CleanUpFn m_CleanUpFn;

	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;
	std::vector<ResourceEntry<KeyType, ResourceType>> m_ResourceEntries;
	std::unordered_map<KeyType, ID, Hasher> m_KeyIDMap;
	size_t m_unMaxSize = 0;

	std::vector<ID> m_FreeIDs;
	ID m_NextID = 0;

	mutable std::recursive_mutex m_mtxTable;

};

using MaterialTable = ResourceTable<std::string, IMaterial>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized TextureTable

template <typename KeyType>
struct ResourceEntry<KeyType, Texture>{
	using ResourcePtr = std::shared_ptr<Texture>;

	ResourcePtr pResource;
	int32 nRefCount = 0;
	bool bAlive = false;

	KeyType key{};
	uint64 un64DescriptorIndex = std::numeric_limits<uint64>::max();
};

template<typename KeyType>
class ResourceTable<KeyType, Texture> {
public:
	friend class ResourceHandle<KeyType, Texture>;

	using ResourcePtr = typename ResourceEntry<KeyType, Texture>::ResourcePtr;
	using ID = uint64_t;
	using Handle = ResourceHandle<KeyType, Texture>;
	constexpr static ID InvalidID = INVALID_ID;

	struct ResourceDesc {
		enum class TYPE { SRV, UAV, RTV, DSV };
		enum class DIMENSION { TEXTURE2D, TEXTURE3D, TEXTURE2DARRAY, TEXTURECUBE };


		TYPE eType;
		DIMENSION eDimension;
		union {
			D3D12_SHADER_RESOURCE_VIEW_DESC srv;
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
			D3D12_RENDER_TARGET_VIEW_DESC rtv;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv;
		};

		void* pAdditionalData;
	};

public:
	// Initialize
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, size_t nMaxSize, bool bUseDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, D3D12_DESCRIPTOR_HEAP_FLAGS d3dHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
		std::lock_guard lock{ m_mtxTable };
		m_pd3dDeviceRef = pd3dDevice;
		m_unMaxSize = nMaxSize;

		if (bUseDescriptorHeap) {
			D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
			{
				d3dHeapDesc.NumDescriptors = nMaxSize;
				d3dHeapDesc.Type = d3dHeapType;
				d3dHeapDesc.Flags = d3dHeapFlags;
				d3dHeapDesc.NodeMask = 0;
			}

			pd3dDevice->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
		}

		m_ResourceEntries.resize(nMaxSize);
		m_KeyIDMap.reserve(nMaxSize);

		m_bShaderVisible = (d3dHeapFlags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ? true : false;

		m_d3dHeapType = d3dHeapType;
	}

	// Register
	Handle Register(const KeyType& key, ResourcePtr pResource, OUT ResourceDesc* pResourceDesc = nullptr, const void* pContext = nullptr, size_t nContextSize = 0) {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it != m_KeyIDMap.end()) {
			return { this, it->second };
		}

		ID id = AllocateID();
		if (id == INVALID_ID) {
			return {};
		}

		auto& entry = m_ResourceEntries[id];
		entry.pResource = pResource;
		entry.nRefCount = 0;
		entry.bAlive = true;
		entry.key = key;
		entry.un64DescriptorIndex = id;

		if (pResource != nullptr && pResourceDesc != nullptr) {	
			RegisterView(pResource->GetResourcePtr(), id, pResourceDesc, pContext, nContextSize);
		}

		m_KeyIDMap.emplace(key, id);
		return { this, id };
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByHandle(const Handle& handle) const {
		if (handle.GetID() >= m_ResourceEntries.size()) {
			assert(false, "Descriptor overflow");
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
		}

		if (!m_ResourceEntries[handle.GetID()].bAlive) {
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuhandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		uint32 nDescriptorInc = 0;
		switch (m_d3dHeapType)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		{
			nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
			break;
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		{
			nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::RTV);
			break;
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		{
			nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::DSV);
			break;
		}
		default:
		{
			__debugbreak();
			break;
		}
		}

		cpuhandle.Offset(m_ResourceEntries[handle.GetID()].un64DescriptorIndex, nDescriptorInc);
		return cpuhandle;
	}


	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByName(const KeyType& key) const {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end())
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{};

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(m_ResourceEntries[it->second].un64DescriptorIndex, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		return handle;
	}

	// Look Up
	Handle GetHandle(const KeyType& key) {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end()) {
			return {};
		}

		return { this, it->second };
	}

	ResourcePtr GetResourceByID(ID id) const {
		if (id >= m_ResourceEntries.size()) {
			return nullptr;
		}

		if (m_ResourceEntries[id].bAlive == false) {
			return nullptr;
		}

		return m_ResourceEntries[id].pResource;
	}

	ResourcePtr GetResourceByHandle(const Handle& handle) const {
		return handle.GetResource();
	}

	const ResourcePtr GetResourceByName(const KeyType& key) const {
		std::lock_guard lock{ m_mtxTable };
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end())
			return nullptr;

		ID id = it->second;
		if (m_ResourceEntries[id].bAlive == false) {
			return nullptr;
		}

		return m_ResourceEntries[id].pResource;
	}

private:
	void RegisterView(
		ComPtr<ID3D12Resource> pd3dResource,
		uint64 id,
		OUT ResourceDesc* pResourceDesc,
		const void* pContext = nullptr,
		size_t nContextSize = 0);

private:
	bool AddRef(ID id) {
		if (id >= m_unMaxSize || id >= m_ResourceEntries.size()) {
			return false;
		}

		auto& entry = m_ResourceEntries[id];
		std::atomic_ref<int32> refCount{ entry.nRefCount };

		int32 current = refCount.load();

		while (current > 0) {
			if (refCount.compare_exchange_weak(current, current + 1)) {
				return true;
			}
		}
		
		std::lock_guard lock{ m_mtxTable };
		if (!entry.bAlive) {
			return false;
		}

		refCount.fetch_add(1);
		return true;
	}

	bool Release(ID id) {
		if (id >= m_unMaxSize || id >= m_ResourceEntries.size()) {
			return false;
		}

		auto& entry = m_ResourceEntries[id];
		std::atomic_ref<int32> refCount{ entry.nRefCount };

		int32 current = refCount.load();

		while (current > 1) {
			if (refCount.compare_exchange_weak(current, current - 1)) {
				return true;
			}
		}

		std::lock_guard lock{ m_mtxTable };
		if (!entry.bAlive) {
			return false;
		}

		const int32 previous = refCount.fetch_sub(1);
		if (previous > 1) {
			return true;
		}

		if (previous != 1) {
			refCount.fetch_add(1);
			assert(false && "Invalud resource ref count");
			return false;
		}

		m_KeyIDMap.erase(entry.key);
		entry.pResource.reset();
		entry.bAlive = false;
		entry.key = KeyType{};
		entry.un64DescriptorIndex = std::numeric_limits<uint64>::max();
		FreeID(id);

		return true;
	}

	ID AllocateID() {
		if (!m_FreeIDs.empty()) {
			ID id = m_FreeIDs.back();
			m_FreeIDs.pop_back();
			return id;
		}

		if (m_NextID >= m_unMaxSize) {
			return InvalidID;
		}

		return m_NextID++;
	}

	void FreeID(ID id) {
		m_FreeIDs.push_back(id);
	}

	bool IsAlive(ID id) const {
		if (id >= m_unMaxSize) {
			return false;
		}
		return m_ResourceEntries[id].bAlive;
	}

private:
	ComPtr<ID3D12Device> m_pd3dDeviceRef = nullptr;

	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;
	std::vector<ResourceEntry<KeyType, Texture>> m_ResourceEntries;
	std::unordered_map<KeyType, ID> m_KeyIDMap;

	size_t m_unMaxSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE m_d3dHeapType;
	bool m_bShaderVisible = false;

	std::vector<ID> m_FreeIDs;
	ID m_NextID = 0;

	mutable std::recursive_mutex m_mtxTable;
};

template<typename KeyType>
inline void ResourceTable<KeyType, Texture>::RegisterView(ComPtr<ID3D12Resource> pd3dResource, uint64 id, OUT ResourceDesc* pResourceDesc, const void* pContext, size_t nContextSize)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = pd3dResource->GetDesc();
	switch (m_d3dHeapType) {
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
	{
		if (pResourceDesc->eType == ResourceDesc::TYPE::SRV) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			{
				srvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				switch (pResourceDesc->eDimension)
				{
				case ResourceDesc::DIMENSION::TEXTURE2D:
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = d3dResourceDesc.MipLevels;
					srvDesc.Texture2D.MostDetailedMip = 0;
					srvDesc.Texture2D.PlaneSlice = 0;
					srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
					break;
				}
				case ResourceDesc::DIMENSION::TEXTURE2DARRAY:
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.MipLevels = d3dResourceDesc.MipLevels;
					srvDesc.Texture2DArray.MostDetailedMip = 0;
					srvDesc.Texture2DArray.PlaneSlice = 0;
					srvDesc.Texture2DArray.ResourceMinLODClamp = 0.f;
					srvDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
					srvDesc.Texture2DArray.FirstArraySlice = 0;
					break;
				}
				case ResourceDesc::DIMENSION::TEXTURECUBE:
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srvDesc.TextureCube.MipLevels = d3dResourceDesc.MipLevels;
					srvDesc.TextureCube.MostDetailedMip = 0;
					srvDesc.TextureCube.ResourceMinLODClamp = 0.f;
					break;
				}
				case ResourceDesc::DIMENSION::TEXTURE3D:
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
					srvDesc.Texture3D.MipLevels = d3dResourceDesc.MipLevels;
					srvDesc.Texture3D.MostDetailedMip = 0;
					srvDesc.Texture3D.ResourceMinLODClamp = 0.f;
					break;
				}
				default:
					break;
				}
			}
			memcpy(&pResourceDesc->srv, &srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

			CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			SRVHandle.Offset(id, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			m_pd3dDeviceRef->CreateShaderResourceView(pd3dResource.Get(), &srvDesc, SRVHandle);
		}
		else {

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			{
				uavDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
				switch (pResourceDesc->eDimension)
				{
				case ResourceDesc::DIMENSION::TEXTURE2D:
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uavDesc.Texture2D.MipSlice = 0;
					uavDesc.Texture2D.PlaneSlice = 0;
					break;
				}
				case ResourceDesc::DIMENSION::TEXTURE2DARRAY:
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uavDesc.Texture2DArray.MipSlice = 0;
					uavDesc.Texture2DArray.PlaneSlice = 0;
					uavDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
					uavDesc.Texture2DArray.FirstArraySlice = 0;
					break;
				}
				case ResourceDesc::DIMENSION::TEXTURE3D:
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					uavDesc.Texture3D.FirstWSlice = 0;
					uavDesc.Texture3D.MipSlice = 0;
					uavDesc.Texture3D.WSize = *(int*)(pResourceDesc->pAdditionalData);
					break;
				}
				default:
					break;
				}
			}
			memcpy(&pResourceDesc->uav, &uavDesc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			UAVHandle.Offset(id, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			m_pd3dDeviceRef->CreateUnorderedAccessView(pd3dResource.Get(), nullptr, &uavDesc, UAVHandle);
		}
		break;
	}
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		{
			rtvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
		}
		memcpy(&pResourceDesc->rtv, &rtvDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		RTVHandle.Offset(id, D3DCore::g_nRTVDescriptorIncrementSize);
		m_pd3dDeviceRef->CreateRenderTargetView(pd3dResource.Get(), &rtvDesc, RTVHandle);

		break;
	}
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		{
			dsvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		}
		memcpy(&pResourceDesc->dsv, &dsvDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

		CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		DSVHandle.Offset(id, D3DCore::g_nDSVDescriptorIncrementSize);
		m_pd3dDeviceRef->CreateDepthStencilView(pd3dResource.Get(), &dsvDesc, DSVHandle);

		break;
	}
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
	default:
	{
		std::unreachable();
		break;
	}
	}

}

using TextureTable = ResourceTable<std::string, Texture>;
