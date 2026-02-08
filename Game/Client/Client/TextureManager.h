#pragma once
#include "Texture.h"

//#define ASSERT_WHEN_TEXTURE_NAME_IS_BLANK

constexpr static UINT MAX_TEXTURE_COUNT = 200;

class TextureManager {
	
	DECLARE_SINGLE(TextureManager)

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);

	void LoadGameTextures();

public:
	std::shared_ptr<Texture> LoadTexture(const std::string& strTextureName);
	std::shared_ptr<Texture> LoadTextureFromRaw(const std::string& strTextureName, uint32 unWidth, uint32 unHeight);
	std::shared_ptr<Texture> LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath);
	
	std::shared_ptr<Texture> GetByName(const std::string& strTextureName) const;
	std::shared_ptr<Texture> GetByID(uint64 unID) const;

	void WaitForCopyComplete();

	D3D12_CPU_DESCRIPTOR_HANDLE CreateSRV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_SHADER_RESOURCE_VIEW_DESC& outViewDesc);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_RENDER_TARGET_VIEW_DESC& outViewDesc);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateDSV(const ShaderResource& texResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_DEPTH_STENCIL_VIEW_DESC& outViewDesc);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateUAV(const ShaderResource& texResource, ComPtr<ID3D12Resource> pCounterResource, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_UNORDERED_ACCESS_VIEW_DESC& outViewDesc);

	void UpdateResources(
		ShaderResource& texResource,
		const std::vector<D3D12_SUBRESOURCE_DATA>& subResources,
		uint32 unBytes,
		ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr);


	D3D12_CPU_DESCRIPTOR_HANDLE RegisterSRV(std::shared_ptr<Texture> pTexture, const DXGI_FORMAT* pdxgiFormat, OUT D3D12_SHADER_RESOURCE_VIEW_DESC& outViewDesc);

private:
	void ReleaseCompletedUploadBuffers();
	void CreateUploadBuffer(ID3D12Resource** ppUploadBuffer, uint32 unBytes);

private:
	TextureTable m_SRVTextureTable;
	TextureTable m_RTVTextureTable;
	TextureTable m_DSVTextureTable;

private:
	CommandListPool						m_CommandListPool;
	std::vector<PendingUploadBuffer>	m_PendingUploadBuffers;

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

	DescriptorHeap m_SRVUAVDescriptorHeap{};
	DescriptorHeap m_RTVDescriptorHeap{};
	DescriptorHeap m_DSVDescriptorHeap{};

	UINT m_nNumSRVUAVTextures = 0;
	UINT m_nNumRTVTextures = 0;
	UINT m_nNumDSVTextures = 0;
#pragma endregion

};

