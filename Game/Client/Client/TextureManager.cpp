#include "pch.h"
#include "TextureManager.h"

void TextureManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	CreateCommandList();
	CreateFence();

	//// Create DescriptorHeaps + Table
	m_SRVTextureTable.Initialize(MAX_TEXTURE_COUNT, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_UAVTextureTable.Initialize(50, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_RTVTextureTable.Initialize(50, true, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	m_DSVTextureTable.Initialize(50, true, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	m_CommandListPool.Initialize(pd3dDevice);
}

void TextureManager::LoadGameTextures()
{
	// Font
	LoadTexture("font");
}

Texture::ID TextureManager::LoadTexture(const std::string& strTextureName)
{
	Texture::ID pFind = m_SRVTextureTable.GetID(strTextureName);
	if (pFind == TextureTable::InvalidID) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		bool bResult = pTexture->CreateTextureFromFile(::StringToWString(strTextureName));
		if (!bResult) {
			return TextureTable::InvalidID;
		}

		uint64 un64TexID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
		if (un64TexID == TextureTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return TextureTable::InvalidID;
		}

		pTexture->m_un64RuntimeSRVID = un64TexID;
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByID(pTexture->m_un64RuntimeSRVID);

		return un64TexID;
	}

	return pFind;
}

Texture::ID TextureManager::LoadTextureFromRaw(const std::string& strTextureName, uint32 unWidth, uint32 unHeight)
{
	Texture::ID pFind = m_SRVTextureTable.GetID(strTextureName);
	if (pFind == TextureTable::InvalidID) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		bool bResult = pTexture->CreateTextureFromRawFile(::StringToWString(strTextureName));
		if (!bResult) {
			return TextureTable::InvalidID;
		}

		uint64 un64TexID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
		if (un64TexID == TextureTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return TextureTable::InvalidID;
		}

		pTexture->m_un64RuntimeSRVID = un64TexID;
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByID(pTexture->m_un64RuntimeSRVID);

		return un64TexID;
	}

	return pFind;
}

Texture::ID TextureManager::LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath)
{
	Texture::ID pFind = m_SRVTextureTable.GetID(strTextureName);
	if (pFind == TextureTable::InvalidID) {
		std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
		bool bResult = pTexture->CreateTextureArrayFromFile(::StringToWString(strTextureName));
		if (!bResult) {
			return TextureTable::InvalidID;
		}

		uint64 un64TexID = m_SRVTextureTable.Register(strTextureName, pTexture, (void*)&pTexture->GetSRVDesc(), sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
		if (un64TexID == TextureTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load texture : {}", strTextureName).c_str());
			return TextureTable::InvalidID;
		}

		pTexture->m_un64RuntimeSRVID = un64TexID;
		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByID(pTexture->m_un64RuntimeSRVID);

		return un64TexID;
	}

	return pFind;
}

std::pair<Texture::ID, Texture::ID> TextureManager::LoadRenderTargetTexture(const std::string& strTextureName, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat)
{
	Texture::ID un64SRVFindID = m_SRVTextureTable.GetID(strTextureName);
	if (un64SRVFindID == TextureTable::InvalidID) {
		std::shared_ptr<RenderTargetTexture> pTexture = std::make_shared<RenderTargetTexture>();
		bool bResult = pTexture->Initialize(unWidth, unHeight, dxgiSRVFormat, dxgiRTVFormat);;
		if (!bResult) {
			return { TextureTable::InvalidID, TextureTable::InvalidID };
		}

		uint64 un64SRVTexID = m_SRVTextureTable.Register(
			strTextureName, 
			pTexture, 
			(void*)&pTexture->GetSRVDesc(), sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC), 
			&dxgiSRVFormat, sizeof(DXGI_FORMAT));

		if (un64SRVTexID == TextureTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load texture SRV : {}", strTextureName).c_str());
			return { TextureTable::InvalidID, TextureTable::InvalidID };
		}

		pTexture->m_d3dSRVHandle = m_SRVTextureTable.GetCPUHandleByID(pTexture->m_un64RuntimeSRVID);

		uint64 un64RTVTexID = m_RTVTextureTable.Register(
			strTextureName, 
			pTexture, 
			(void*)&pTexture->GetRTVDesc(), sizeof(D3D12_RENDER_TARGET_VIEW_DESC),
			&dxgiRTVFormat, sizeof(DXGI_FORMAT));

		if (un64RTVTexID == TextureTable::InvalidID) {
			OutputDebugStringA(std::format("Failed to load texture RTV : {}", strTextureName).c_str());
			return { TextureTable::InvalidID, TextureTable::InvalidID };
		}

		pTexture->m_un64RuntimeRTVID = un64RTVTexID;
		pTexture->m_d3dRTVHandle = m_RTVTextureTable.GetCPUHandleByID(pTexture->m_un64RuntimeRTVID);

		return { un64SRVTexID, un64RTVTexID };
	}

	Texture::ID un64RTVFindID = m_RTVTextureTable.GetID(strTextureName);

	return { un64SRVFindID, un64RTVFindID };
}

std::shared_ptr<Texture> TextureManager::GetTextureByName(const std::string& strTextureName, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	std::shared_ptr<Texture> pTexture = nullptr;

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		pTexture = m_SRVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		pTexture = m_RTVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		pTexture = m_UAVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		pTexture = m_DSVTextureTable.GetResourceByName(strTextureName);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return pTexture;
}

std::shared_ptr<Texture> TextureManager::GetTextureByID(uint64 unID, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	std::shared_ptr<Texture> pTexture = nullptr;

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		pTexture = m_SRVTextureTable.GetResourceByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		pTexture = m_RTVTextureTable.GetResourceByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		pTexture = m_UAVTextureTable.GetResourceByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		pTexture = m_DSVTextureTable.GetResourceByID(unID);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return pTexture;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCPUHandleByID(uint64 unID, TEXTURE_RESOURCE_TYPE eResourceType) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{};

	switch (eResourceType)
	{
	case TEXTURE_RESOURCE_TYPE::SRV:
	{
		CPUHandle = m_SRVTextureTable.GetCPUHandleByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::RTV:
	{
		CPUHandle = m_RTVTextureTable.GetCPUHandleByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::UAV:
	{
		CPUHandle = m_UAVTextureTable.GetCPUHandleByID(unID);
		break;
	}
	case TEXTURE_RESOURCE_TYPE::DSV:
	{
		CPUHandle = m_DSVTextureTable.GetCPUHandleByID(unID);
		break;
	}
	default:
	{
		std::unreachable();
	}
	}

	return CPUHandle;
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

