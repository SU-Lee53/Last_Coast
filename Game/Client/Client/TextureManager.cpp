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
	auto pTexture = m_SRVTextureTable.GetResourceByName(strTextureName);
	if (!pTexture) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		pTexture->CreateTextureFromFile(::StringToWString(strTextureName));
		if (pTexture) {
			pTexture->m_un64RuntimeID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), nullptr);
		}
	}

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::LoadTextureFromRaw(const std::string& strTextureName, uint32 unWidth, uint32 unHeight)
{
	auto pTexture = m_SRVTextureTable.GetResourceByName(strTextureName);
	if (!pTexture) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		pTexture->CreateTextureFromRawFile(::StringToWString(strTextureName), unWidth, unHeight);
		if (pTexture) {
			pTexture->m_un64RuntimeID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), nullptr);
		}
	}

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath)
{
	auto pTexture = m_SRVTextureTable.GetResourceByName(strTextureName);
	if (!pTexture) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		pTexture->CreateTextureArrayFromFile(wstrTexturePath);
		if (pTexture) {
			pTexture->m_un64RuntimeID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), nullptr);
		}
	}

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::GetByName(const std::string& strTextureName) const
{
	return m_SRVTextureTable.GetByName(strTextureName);
}

std::shared_ptr<Texture> TextureManager::GetByID(uint64 unID) const
{
	return m_SRVTextureTable.GetByID(unID);
}

void TextureManager::WaitForCopyComplete()
{
	while (m_PendingUploadBuffers.size() != 0) {
		ReleaseCompletedUploadBuffers();
	}
}

void TextureManager::CreateUploadBuffer(ID3D12Resource** ppUploadBuffer, uint32 unBytes)
{
	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(unBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(ppUploadBuffer)
	);

	if (FAILED(hr)) {
		__debugbreak();
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

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CreateSRV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_SHADER_RESOURCE_VIEW_DESC& outViewDesc)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = texResource.pResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	{
		srvDesc.Format = pdxgiFormat ? *pdxgiFormat : d3dResourceDesc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = d3dResourceDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	outViewDesc = srvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE SRVHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	SRVHandle.Offset(m_nNumSRVUAVTextures++, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	m_pd3dDevice->CreateShaderResourceView(texResource.pResource.Get(), &srvDesc, SRVHandle);
	return SRVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CreateRTV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_RENDER_TARGET_VIEW_DESC& outViewDesc)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = texResource.pResource->GetDesc();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	{
		rtvDesc.Format = pdxgiFormat ? *pdxgiFormat : d3dResourceDesc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
	}
	outViewDesc = rtvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle = m_RTVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	RTVHandle.Offset(m_nNumRTVTextures++, D3DCore::g_nRTVDescriptorIncrementSize);
	m_pd3dDevice->CreateRenderTargetView(texResource.pResource.Get(), &rtvDesc, RTVHandle);

	return RTVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CreateDSV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_DEPTH_STENCIL_VIEW_DESC& outViewDesc)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = texResource.pResource->GetDesc();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	{
		dsvDesc.Format = pdxgiFormat ? *pdxgiFormat : d3dResourceDesc.Format;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	outViewDesc = dsvDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE DSVHandle = m_DSVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	DSVHandle.Offset(m_nNumRTVTextures++, D3DCore::g_nDSVDescriptorIncrementSize);
	m_pd3dDevice->CreateDepthStencilView(texResource.pResource.Get(), &dsvDesc, DSVHandle);

	return DSVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::CreateUAV(const ShaderResource& texResource, ComPtr<ID3D12Resource> pCounterResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_UNORDERED_ACCESS_VIEW_DESC& outViewDesc)
{
	D3D12_RESOURCE_DESC d3dResourceDesc = texResource.pResource->GetDesc();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	{
		uavDesc.Format = pdxgiFormat ? *pdxgiFormat : d3dResourceDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;
	}
	outViewDesc = uavDesc;

	CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle = m_SRVUAVDescriptorHeap.GetDescriptorHandleFromHeapStart().cpuHandle;
	UAVHandle.Offset(m_nNumSRVUAVTextures++, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	m_pd3dDevice->CreateUnorderedAccessView(texResource.pResource.Get(), pCounterResource.Get(), &uavDesc, UAVHandle);
	return UAVHandle;
}

void TextureManager::UpdateResources(ShaderResource& texResource, const std::vector<D3D12_SUBRESOURCE_DATA>& subResources, uint32 unBytes, ComPtr<ID3D12Resource> pd3dUploadBuffer)
{
	if (!pd3dUploadBuffer) {
		CreateUploadBuffer(pd3dUploadBuffer.GetAddressOf(), unBytes);
	}

	// BinaryResource -> Upload Buffer -> Texture Buffer
	auto cmdList = AllocateCommandListSafe();
	{
		texResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
		{
			::UpdateSubresources(cmdList->pd3dCommandList.Get(), texResource.pResource.Get(), pd3dUploadBuffer.Get(), 0, 0, subResources.size(), subResources.data());
		}
		texResource.StateTransition(cmdList->pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
	ExcuteCommandList(*cmdList);
	cmdList->ui64FenceValue = Fence();
	m_PendingUploadBuffers.push_back({ pd3dUploadBuffer, cmdList });
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

