#pragma once
#include "Texture.h"

//#define ASSERT_WHEN_TEXTURE_NAME_IS_BLANK

constexpr static UINT MAX_TEXTURE_COUNT = 300;

enum class TEXTURE_RESOURCE_TYPE {
	SRV,
	RTV,
	UAV,
	DSV
};

class TextureManager {
	
	DECLARE_SINGLE(TextureManager)

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);

	void LoadGameTextures();

public:
	TextureRef<Texture> LoadTexture(
		const std::string& strTextureName,
		bool bCheckTransparent = false);

	TextureRef<Texture> LoadTextureFromRaw(
		const std::string& strTextureName, 
		uint32 unWidth, 
		uint32 unHeight);

	TextureRef<Texture> LoadTextureArray(
		const std::string& strTextureName, 
		const std::wstring& wstrTexturePath);
	
	TextureRef<Texture> LoadTextureFromRawData(
		const std::string& strTextureName, 
		const std::vector<Vector4>& data,
		uint32 unWidth, 
		uint32 unHeight,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN);
	
	TextureRef<Texture> LoadTextureFromHeightData(
		const std::string& strTextureName, 
		const std::vector<uint16>& data,
		uint32 unWidth, 
		uint32 unHeight);

	TextureRef<RenderTargetTexture> LoadRenderTargetTexture(
		const std::string& strTextureName, 
		uint32 unWidth,
		uint32 unHeight,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN,
		D3D12_RESOURCE_STATES d3dInitialState = D3D12_RESOURCE_STATE_PRESENT,
		float* pfClearValue = nullptr);
	
	TextureRef<RenderTargetTexture> LoadRenderTargetTexture(
		ComPtr<ID3D12Resource> pd3dRTVResourceFromSwapChain,
		DXGI_FORMAT dxgiSRVFormat,
		DXGI_FORMAT dxgiRTVFormat);

	TextureRef<DepthStencilTexture> LoadDepthStencilTexture(
		const std::string& strTextureName,
		uint32 unWidth,
		uint32 unHeight,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiDSVFormat = DXGI_FORMAT_UNKNOWN);
	
	TextureRef<UnorderedAccessTexture> LoadUnorderedAccessTexture(
		const std::string& strTextureName,
		uint32 unArraySize,
		uint32 unWidth,
		uint32 unHeight,
		uint32 unDepth = 0,
		DXGI_FORMAT dxgiSRVUAVFormat = DXGI_FORMAT_UNKNOWN);
	
	
	TextureRef<RWRenderTargetTexture> LoadRWRenderTargetTexture(
		const std::string& strTextureName,
		uint32 unWidth,
		uint32 unHeight,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiUAVFormat = DXGI_FORMAT_UNKNOWN);
	
	std::shared_ptr<Texture> GetTextureByName(const std::string& strTextureName, TEXTURE_RESOURCE_TYPE eResourceType) const;
	std::shared_ptr<Texture> GetTextureByHandle(const TextureHandle& handle, TEXTURE_RESOURCE_TYPE eResourceType) const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByHandle(const TextureHandle& handle, TEXTURE_RESOURCE_TYPE eResourceType) const;


	void WaitForCopyComplete();

	void UpdateResources(
		ComPtr<ID3D12Resource> pResource,
		D3D12_RESOURCE_STATES d3dCurrentState,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subResources,
		uint32 unBytes,
		ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr);

private:
	void ReleaseCompletedUploadBuffers();
	void CreateUploadBuffer(ID3D12Resource** ppUploadBuffer, uint32 unBytes);

private:
	TextureTable m_SRVTextureTable;
	TextureTable m_RTVTextureTable;
	TextureTable m_UAVTextureTable;
	TextureTable m_DSVTextureTable;

private:
	CommandListPool						m_CommandListPool;
	std::vector<PendingUploadBuffer>	m_PendingUploadBuffers;

	static uint32 g_unRTVFromCoreCount;
	static uint32 g_unDSVFromCoreCount;

	TextureRef<Texture> m_DebugAlbedo{};
	TextureRef<Texture> m_DebugNormal{};

#pragma region D3D
private:
	void CreateCommandList();
	void CreateFence();
	UINT64 Fence();
	void WaitForGPUComplete();

	void ExcuteCommandList(CommandListPair& cmdPair);
	CommandListPair* AllocateCommandListSafe();	// Helper

private:
	ComPtr<ID3D12Device>				m_pd3dDevice = nullptr;		// Reference to D3DCore::m_pd3dDevice
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;

	ComPtr<ID3D12Fence>		m_pd3dFence = nullptr;
	HANDLE					m_hFenceEvent = nullptr;
	UINT64					m_nFenceValue = 0;

#pragma endregion

};
