#include "pch.h"
#include "TextureManager.h"

void TextureManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	CreateCommandList();
	CreateFence();

	// Create DescriptorHeaps
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	{
		heapDesc.NumDescriptors = MAX_TEXTURE_COUNT;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
	}
	m_SRVUAVDescriptorHeap.Initialize(pd3dDevice, heapDesc);

	{
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	}
	m_RTVDescriptorHeap.Initialize(pd3dDevice, heapDesc);

	{
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	}
	m_DSVDescriptorHeap.Initialize(pd3dDevice, heapDesc);

	m_CommandListPool.Initialize(pd3dDevice);
}

void TextureManager::LoadGameTextures()
{
	// Font
	LoadTexture("font");
}

std::shared_ptr<Texture> TextureManager::LoadTexture(const std::string& strTextureName)
{
	auto it = m_pTexturePool.find(strTextureName);
	if (it == m_pTexturePool.end()) {
		std::shared_ptr<Texture> pTexture = CreateTextureFromFile(::StringToWString(strTextureName));
		if (pTexture) {
			m_pTexturePool[strTextureName] = pTexture;
		}
	}

	return m_pTexturePool[strTextureName];
}

std::shared_ptr<Texture> TextureManager::LoadTextureFromRaw(const std::string& strTextureName, uint32 unWidth, uint32 unHeight)
{
	auto it = m_pTexturePool.find(strTextureName);
	if (it == m_pTexturePool.end()) {
		std::shared_ptr<Texture> pTexture = CreateTextureFromRawFile(::StringToWString(strTextureName), unWidth, unHeight);
		if (pTexture) {
			m_pTexturePool[strTextureName] = pTexture;
		}
	}

	return m_pTexturePool[strTextureName];
}

std::shared_ptr<Texture> TextureManager::LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath)
{
	auto it = m_pTexturePool.find(strTextureName);
	if (it == m_pTexturePool.end()) {
		std::shared_ptr<Texture> pTexture = CreateTextureArrayFromFile(wstrTexturePath);
		if (pTexture) {
			m_pTexturePool[strTextureName] = pTexture;
		}
	}

	return m_pTexturePool[strTextureName];
}

std::shared_ptr<Texture> TextureManager::Get(const std::string& strTextureName) const
{
	auto it = m_pTexturePool.find(strTextureName);
	if (it != m_pTexturePool.end()) {
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<Texture> TextureManager::CreateTextureFromFile(const std::wstring& wstrTextureName)
{
	namespace fs = std::filesystem;
	
	std::wstring wstrTexturePath;
	if (fs::path(wstrTextureName).has_extension()) {
		wstrTexturePath = wstrTextureName;
	}
	else {
		wstrTexturePath = std::format(L"{}/{}.dds", g_wstrTextureBasePath, wstrTextureName);
	}


	fs::path texPath{ wstrTexturePath };
	if (!fs::exists(texPath)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", texPath.string()).c_str());
		return nullptr;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	if (texPath.extension().string() == ".dds" || texPath.extension().string() == ".DDS") {
		LoadFromDDSFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subresources);
	}
	else {
		LoadFromWICFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subresources);
	}

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT nSubResources = (UINT)subresources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;
	//	UINT nSubResources = d3dTextureResourceDesc.DepthOrArraySize * d3dTextureResourceDesc.MipLevels;
	//	UINT64 nBytes = 0;
	//	pd3dDevice->GetCopyableFootprints(&d3dTextureResourceDesc, 0, nSubResources, 0, NULL, NULL, NULL, &nBytes);

	ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr;
	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pd3dUploadBuffer.GetAddressOf())
	);

	// BinaryResource -> Upload Buffer -> Texture Buffer
	auto cmdList = AllocateCommandListSafe();
	{
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			::UpdateSubresources(cmdList->pd3dCommandList.Get(), pTexture->m_pTexResource.pResource.Get(), pd3dUploadBuffer.Get(), 0, 0, nSubResources, subresources.data());
		}
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
	ExcuteCommandList(*cmdList);
	cmdList->ui64FenceValue = Fence();
	m_PendingUploadBuffers.push_back({ pd3dUploadBuffer, cmdList });

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	{
		srvDesc.Format = d3dTextureResourceDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = d3dTextureResourceDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	pTexture->m_d3dSRVDesc = srvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	SRVHandle.Offset(m_nNumSRVUAVTextures++, D3DCORE->g_nCBVSRVDescriptorIncrementSize);
	m_pd3dDevice->CreateShaderResourceView(pTexture->m_pTexResource.pResource.Get(), &srvDesc, SRVHandle);
	pTexture->m_d3dSRVHandle = SRVHandle;

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::CreateTextureArrayFromFile(const std::wstring& wstrTexturePath)
{
	namespace fs = std::filesystem;

	fs::path texPath{ wstrTexturePath };
	if (!fs::exists(texPath)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", texPath.string()).c_str());
		return nullptr;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	if (texPath.extension().string() == ".dds" || texPath.extension().string() == ".DDS") {
		LoadFromDDSFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subresources);
	}
	else {
		LoadFromWICFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subresources);
	}

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT nSubResources = (UINT)subresources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;
	//	UINT nSubResources = d3dTextureResourceDesc.DepthOrArraySize * d3dTextureResourceDesc.MipLevels;
	//	UINT64 nBytes = 0;
	//	pd3dDevice->GetCopyableFootprints(&d3dTextureResourceDesc, 0, nSubResources, 0, NULL, NULL, NULL, &nBytes);

	ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr;
	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pd3dUploadBuffer.GetAddressOf())
	);

	// BinaryResource -> Upload Buffer -> Texture Buffer
	auto cmdList = AllocateCommandListSafe();
	{
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			::UpdateSubresources(cmdList->pd3dCommandList.Get(), pTexture->m_pTexResource.pResource.Get(), pd3dUploadBuffer.Get(), 0, 0, nSubResources, subresources.data());
		}
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
	ExcuteCommandList(*cmdList);
	cmdList->ui64FenceValue = Fence();
	m_PendingUploadBuffers.push_back({ pd3dUploadBuffer, cmdList });

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	{
		srvDesc.Format = d3dTextureResourceDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = d3dTextureResourceDesc.MipLevels;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = d3dTextureResourceDesc.DepthOrArraySize;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
	}
	pTexture->m_d3dSRVDesc = srvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	SRVHandle.Offset(m_nNumSRVUAVTextures++);
	m_pd3dDevice->CreateShaderResourceView(pTexture->m_pTexResource.pResource.Get(), &srvDesc, SRVHandle);
	pTexture->m_d3dSRVHandle = SRVHandle;

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat)
{
	// TODO : std::vector 생명주기와 GPU 복사 시기를 고려해야 함
	namespace fs = std::filesystem;

	//std::wstring wstrTexturePath;
	std::ifstream in{ wstrTexturePath, std::ios::binary };
	if (!in) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", fs::path(wstrTexturePath).string()).c_str());
		__debugbreak();
		return nullptr;
	}

	// 파일 읽기
	in.seekg(0, std::ios::end);
	int32 nSize = in.tellg();
	in.seekg(0, std::ios::beg);

	std::vector<uint8> rawData;
	rawData.resize(nSize);
	in.read((char*)rawData.data(), nSize);

	// 리소스 포인터 생성
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = unWidth;
	resourceDesc.Height = unHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = dxgiFormat;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	CD3DX12_HEAP_PROPERTIES d3dHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	pTexture->m_pTexResource.Create(
		m_pd3dDevice,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr
	);

	// UploadBuffer 생성
	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, 1);

	ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr;
	m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pd3dUploadBuffer.GetAddressOf())
	);

	// UploadBuffer 에 바로 Raw data 복사
	uint8* pMappedPtr = nullptr;
	CD3DX12_RANGE d3dReadRange(0, 0);
	pd3dUploadBuffer->Map(0, &d3dReadRange, reinterpret_cast<void**>(&pMappedPtr));
	::memcpy(pMappedPtr, rawData.data(), rawData.size());
	pd3dUploadBuffer->Unmap(0, nullptr);

	D3D12_SUBRESOURCE_DATA subresourceData{};
	subresourceData.pData = pMappedPtr;
	subresourceData.RowPitch = unWidth * sizeof(uint32); // R8G8B8A8
	subresourceData.SlicePitch = subresourceData.RowPitch * unHeight;

	// Raw Data -> Tex Resource 로 복사
	auto cmdList = AllocateCommandListSafe();
	{
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			::UpdateSubresources(cmdList->pd3dCommandList.Get(), pTexture->m_pTexResource.pResource.Get(), pd3dUploadBuffer.Get(), 0, 0, 1, &subresourceData);
		}
		pTexture->m_pTexResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	ExcuteCommandList(*cmdList);
	cmdList->ui64FenceValue = Fence();
	m_PendingUploadBuffers.push_back({ pd3dUploadBuffer, cmdList });

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	{
		srvDesc.Format = d3dTextureResourceDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = d3dTextureResourceDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	pTexture->m_d3dSRVDesc = srvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	SRVHandle.Offset(m_nNumSRVUAVTextures++);
	m_pd3dDevice->CreateShaderResourceView(pTexture->m_pTexResource.pResource.Get(), &srvDesc, SRVHandle);
	pTexture->m_d3dSRVHandle = SRVHandle;

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::CreateRTVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat)
{
	if (auto pTex = m_pTexturePool.find(strName); pTex != m_pTexturePool.end()) {
		return pTex->second;
	}

	std::shared_ptr<RenderTargetTexture> pTexture = std::make_shared<RenderTargetTexture>();
	pTexture->Initialize(m_pd3dDevice, uiWidth, uiHeight, DXGI_FORMAT_UNKNOWN);
	
	D3D12_RESOURCE_DESC resourceDesc = pTexture->m_pTexResource.pResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	{
		srvDesc.Format = dxgiSRVFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	CPUHandle.Offset(m_nNumSRVUAVTextures++);
	m_pd3dDevice->CreateShaderResourceView(pTexture->m_pTexResource.pResource.Get(), &srvDesc, CPUHandle);
	pTexture->m_d3dSRVHandle = CPUHandle;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	{
		rtvDesc.Format = dxgiRTVFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
	}

	CPUHandle = m_RTVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	CPUHandle.Offset(m_nNumRTVTextures++);
	m_pd3dDevice->CreateRenderTargetView(pTexture->m_pTexResource.pResource.Get(), &rtvDesc, CPUHandle);
	pTexture->m_d3dRTVHandle = CPUHandle;

	m_pTexturePool.insert({strName, pTexture});

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::CreateUAVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiFormat)
{
	// TODO : 구현
	return std::shared_ptr<Texture>();
}

std::shared_ptr<Texture> TextureManager::CreateDSVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiFormat)
{
	// TODO : 구현
	return std::shared_ptr<Texture>();
}

void TextureManager::LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
{
	HRESULT hr;
	DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool bIsCubeMap = false;

	hr = ::LoadDDSTextureFromFileEx(
		m_pd3dDevice.Get(),
		wstrTexturePath.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		DDS_LOADER_DEFAULT,
		ppOutResource,
		ddsData,
		subResources,
		&ddsAlphaMode,
		&bIsCubeMap
	);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {}", __FILE__, __LINE__, "Failed To Load DDS File").c_str());
		return;
	}

}

void TextureManager::LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
{
	HRESULT hr;

	subResources.resize(1);

	hr = ::LoadWICTextureFromFileEx(
		m_pd3dDevice.Get(),
		wstrTexturePath.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		WIC_LOADER_IGNORE_SRGB,
		ppOutResource,
		ddsData,
		subResources[0]
	);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {}", __FILE__, __LINE__, "Failed To Load WIC File").c_str());
		return;
	}
}

void TextureManager::WaitForCopyComplete()
{
	while (m_PendingUploadBuffers.size() != 0) {
		ReleaseCompletedUploadBuffers();
	}
}

void TextureManager::ReleaseCompletedUploadBuffers()
{
	UINT64 ui64CompletedValue = m_pd3dFence->GetCompletedValue();
	m_CommandListPool.ReclaimEnded(ui64CompletedValue);

	std::erase_if(m_PendingUploadBuffers, [](const PendingUploadBuffer& pended) {
		return !pended.cmdListPair->bInUse;
	});
}

#pragma region D3D
void TextureManager::CreateCommandList()
{
	HRESULT hr{};

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}
}

void TextureManager::CreateFence()
{
	HRESULT hr{};

	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create fence");
	}

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void TextureManager::ExcuteCommandList(CommandListPair& cmdPair)
{
	HRESULT hr = cmdPair.pd3dCommandList->Close();
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to close CommandList");
		__debugbreak();
	}


	ID3D12CommandList* ppCommandLists[] = { cmdPair.pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForGPUComplete();
}

UINT64 TextureManager::Fence()
{
	m_nFenceValue++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_nFenceValue);
	return m_nFenceValue;
}

void TextureManager::WaitForGPUComplete()
{
	const UINT64 expectedFenceValue = m_nFenceValue;

	if (m_pd3dFence->GetCompletedValue() < expectedFenceValue)
	{
		m_pd3dFence->SetEventOnCompletion(expectedFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

CommandListPair* TextureManager::AllocateCommandListSafe()
{
	auto cmdList = m_CommandListPool.Allocate(m_nFenceValue);
	if (!cmdList) {
		while (true) {
			UINT64 ui64CompletedValue = m_pd3dFence->GetCompletedValue();
			//m_CommandListPool.ReclaimEnded(ui64CompletedValue);
			ReleaseCompletedUploadBuffers();
			cmdList = m_CommandListPool.Allocate(m_nFenceValue);
			if (cmdList) {
				break;
			}

		}
	}

	return cmdList;
}


#pragma endregion D3D
