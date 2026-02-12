#pragma once


template <typename T>
struct ResourceEntry {
	using ResourcePtr = std::shared_ptr<T>;
	ResourcePtr pResource;
	uint64 un64DescriptorIndex = std::numeric_limits<uint32>::max();
};

template<
	typename KeyType,
	typename ResourceType,
	typename Hash = std::hash<KeyType>,
	typename KeyEqual = std::equal_to<KeyType>
>
class ResourceTable {
public:
	using ResourcePtr = ResourceEntry<ResourceType>::ResourcePtr;
	using ID = ResourceType::ID;
	constexpr static ID InvalidID = std::numeric_limits<ID>::max();

public:
	// Initialize
	void Initialize(size_t nMaxSize, bool bUseDescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, D3D12_DESCRIPTOR_HEAP_FLAGS d3dHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
		m_unMaxSize = nMaxSize;
		if (bUseDescriptorHeap) {
			D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
			{
				d3dHeapDesc.NumDescriptors = nMaxSize;
				d3dHeapDesc.Type = d3dHeapType;
				d3dHeapDesc.Flags = d3dHeapFlags;
				d3dHeapDesc.NodeMask = 0;
			}

			DEVICE->CreateDescriptorHeap(&d3dHeapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
		}

		m_ResourceEntries.reserve(nMaxSize);
		m_KeyIDMap.reserve(nMaxSize);

		m_bShaderVisible = (d3dHeapFlags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) ? true : false;
	}

	// Register
	ID Register(const KeyType& key, ResourcePtr pResource, OUT void* outpView = nullptr, size_t nViewSize = 0, const void* pContext = nullptr, size_t nContextSize = 0) {
		auto it = m_KeyIDMap.find(key);
		if (it != m_KeyIDMap.end()) {
			return it->second;
		}

		ID id = static_cast<ID>(m_ResourceEntries.size());
		if (id > m_unMaxSize) {
			return InvalidID;
		}

		ResourceEntry<ResourceType> entry;
		entry.pResource = pResource;
		entry.un64DescriptorIndex = id;
		if (outpView) {
			RegisterView(pResource->GetResource(), id, outpView, nViewSize, pContext, nContextSize);
		}

		m_ResourceEntries.push_back(entry);
		m_KeyIDMap.emplace(key, id);
		return id;
	}
	
	// Look Up
	ID GetID(const KeyType& key) const {
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end()) {
			return InvalidID;
		}
		return it->second;
	}

	ResourcePtr GetResourceByID(ID id) const {
		if (id >= m_ResourceEntries.size()) {
			return nullptr;
		}

		return m_ResourceEntries[id].pResource;
	}

	ResourcePtr GetResourceByName(const KeyType& key) const {
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end())
			return nullptr;

		return m_ResourceEntries[it->second].pResource;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByID(ID id) const {
		if (id >= m_ResourceEntries.size()) {
			assert(false, "Descriptor overflow");
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(m_ResourceEntries[id].un64DescriptorIndex, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		return handle;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByName(const KeyType& key) const {
		auto it = m_KeyIDMap.find(key);
		if (it == m_KeyIDMap.end())
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{};

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(m_ResourceEntries[it->second].un64DescriptorIndex, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		return handle;
	}

	// Iteration
	auto begin() { return m_ResourceEntries.begin(); }
	auto end() { return m_ResourceEntries.end(); }
	
	auto begin() const { return m_ResourceEntries.begin(); }
	auto end() const { return m_ResourceEntries.end(); }
	
	size_t size() const { return m_ResourceEntries.size(); }

private:
	void RegisterView(
		ComPtr<ID3D12Resource> pd3dResource, 
		uint64 id, 
		OUT void* outpView, 
		size_t nViewSize, 
		const void* pContext = nullptr, 
		size_t nContextSize = 0);

private:
	ComPtr<ID3D12DescriptorHeap> m_pd3dDescriptorHeap;
	std::vector<ResourceEntry<ResourceType>> m_ResourceEntries;
	std::unordered_map<KeyType, ID, Hash, KeyEqual> m_KeyIDMap;

	size_t m_unMaxSize = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE m_d3dHeapType;
	bool m_bShaderVisible = false;

};

using TextureTable = ResourceTable<std::string, class Texture>;
using MaterialTable = ResourceTable<std::string, class IMaterial>;

template<typename KeyType, typename ResourceType, typename Hash, typename KeyEqual>
inline void ResourceTable<KeyType, ResourceType, Hash, KeyEqual>::RegisterView(ComPtr<ID3D12Resource> pd3dResource, uint64 id, OUT void* outpView, size_t nViewSize, const void* pContext, size_t nContextSize)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = pd3dResource->GetDesc();
	switch (m_d3dHeapType) {
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
	{
		if (nViewSize == sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC)) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			{
				srvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = d3dResourceDesc.MipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.PlaneSlice = 0;
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			}
			memcpy(outpView, &srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

			CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			SRVHandle.Offset(id, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			DEVICE->CreateShaderResourceView(pd3dResource.Get(), &srvDesc, SRVHandle);
		}
		else {	// nViewSize == sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC)

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			{
				uavDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = 0;
				uavDesc.Texture2D.PlaneSlice = 0;
			}
			memcpy(outpView, &uavDesc, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			UAVHandle.Offset(id++, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			DEVICE->CreateUnorderedAccessView(pd3dResource.Get(), nullptr, &uavDesc, UAVHandle);

		}
		break;
	}
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		{
			rtvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;
		}
		memcpy(outpView, &rtvDesc, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		RTVHandle.Offset(id, D3DCore::g_nRTVDescriptorIncrementSize);
		DEVICE->CreateRenderTargetView(pd3dResource.Get(), &rtvDesc, RTVHandle);

		break;
	}
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		{
			dsvDesc.Format = pContext ? (*(DXGI_FORMAT*)pContext) : d3dResourceDesc.Format;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
		}
		memcpy(outpView, &dsvDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));

		CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		DSVHandle.Offset(id, D3DCore::g_nDSVDescriptorIncrementSize);
		DEVICE->CreateDepthStencilView(pd3dResource.Get(), &dsvDesc, DSVHandle);

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
