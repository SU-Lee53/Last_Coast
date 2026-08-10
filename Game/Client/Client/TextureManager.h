#pragma once
#include "Texture.h"

//#define ASSERT_WHEN_TEXTURE_NAME_IS_BLANK

/*
*	TextureManager uses 4 kinds of lock
* 
*	Use 2 Lock for loading texture
*		1. Publish Lock -> Lock for table lookup (already loaded) & registry
*		2. Key Lock -> Per key lock for preventing redundant texture loading
* 
*	Use 2 more lock for recording and submitting commandlist
*		1. Submit Lock -> Lock commandlist queue submission
*		2. Fence Lock -> Lock for waiting GPU work when all cmdlists are busy
* 
*	Why per key lock?
*		If two threads comes for loading same texture, this will happen :
*			1. Both threads checks table, but table is empty
*			2. Both thread creates textures
*			3. Only one texture can be register into table, other one will be deleted
*		
*		to prevent this redundant situation, we will use per key lock. and this will happen
*			1. Both threads checks table, but table is empty
*			2. Both thread trying to acquire/create key lock, only one thread can acquire
*			3. Lock acquired thread will load texture, other one will wait
* 
*/


constexpr static UINT MAX_TEXTURE_COUNT = 2048;

enum class TEXTURE_RESOURCE_TYPE {
	SRV,
	RTV,
	UAV,
	DSV
};

class TextureManager {
	
	DECLARE_SINGLE(TextureManager)
	~TextureManager();

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void Shutdown();

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
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_R32G32B32A32_FLOAT);
	
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
		uint64 unBytes,
		ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr);

public:
	void PollCopyComplete();
	bool IsCopyComplete();
	bool IsFenceComplete(uint64 ui64FenceValue) const;
	uint64 GetLastSubmittedFenceValue() const;

private:
	uint64 GetPendingCopyFenceValue() const;

private:
	void ReleaseCompletedUploadBuffers();
	void CreateUploadBuffer(ID3D12Resource** ppUploadBuffer, uint64 unBytes);

	std::shared_ptr<std::mutex> GetTextureLoadMutex(const std::string& strTextureName);

private:
	TextureTable m_SRVTextureTable;
	TextureTable m_RTVTextureTable;
	TextureTable m_UAVTextureTable;
	TextureTable m_DSVTextureTable;

private:
	CommandListPool						m_CommandListPool;

	static uint32 g_unRTVFromCoreCount;
	static uint32 g_unDSVFromCoreCount;

	TextureRef<Texture> m_DebugAlbedo{};
	TextureRef<Texture> m_DebugNormal{};

#pragma region D3D
private:
	void CreateCommandList();
	void CreateFence();
	uint64 Fence();
	void WaitForGPUComplete();

	void ExcuteCommandList(CommandListPair& cmdPair);
	CommandListPair* AllocateCommandListSafe();	// Helper

private:
	ComPtr<ID3D12Device>				m_pd3dDevice = nullptr;		// Reference to D3DCore::m_pd3dDevice
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;

	ComPtr<ID3D12Fence>		m_pd3dFence = nullptr;
	HANDLE					m_hFenceEvent = nullptr;
	uint64					m_un64FenceValue = 0;

	mutable std::recursive_mutex m_mtxTextureLoad; // protects file loading and lookup
	concurrency::concurrent_unordered_map<std::string, std::shared_ptr<std::mutex>> m_TextureLoadMutexRegistry;

	mutable std::mutex m_mtxSubmit;	// lock for queue submission + fence
	mutable std::mutex m_mtxFence;	// lock for fence wait

#pragma endregion

};
