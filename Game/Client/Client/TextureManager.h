#pragma once
#include "Texture.h"

//#define ASSERT_WHEN_TEXTURE_NAME_IS_BLANK

constexpr static UINT MAX_TEXTURE_COUNT = 100;

class TextureManager {
	
	DECLARE_SINGLE(TextureManager)

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);

	void LoadGameTextures();

public:
	std::shared_ptr<Texture> LoadTexture(const std::string& strTextureName);
	std::shared_ptr<Texture> LoadTextureArray(const std::string& strTextureName, const std::wstring& wstrTexturePath);
	std::shared_ptr<Texture> Get(const std::string& strTextureName) const;

public:
	std::shared_ptr<Texture> CreateTextureFromFile(const std::wstring& wstrTexturePath);
	std::shared_ptr<Texture> CreateTextureArrayFromFile(const std::wstring& wstrTexturePath);

public:
	std::shared_ptr<Texture> CreateRTVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN, DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN);
	std::shared_ptr<Texture> CreateUAVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN);
	std::shared_ptr<Texture> CreateDSVTexture(const std::string& strName, UINT uiWidth, UINT uiHeight, DXGI_FORMAT dxgiFormat);

	void WaitForCopyComplete();


private:
	void LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

	// 왠만하면 쓸일 없도록 합시다
	void LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

private:
	void ReleaseCompletedUploadBuffers();

private:
	// Texture Pool
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_pTexturePool;

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

private:
	inline static std::wstring g_wstrTextureBasePath = L"../Resources/Textures";

};

