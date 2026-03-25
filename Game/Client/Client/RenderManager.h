#pragma once
#include "DescriptorHeap.h"

#include "ConstantBufferPool.h"
#include "StructuredBufferPool.h"

#include "RenderGraph.h"

enum class ROOT_PARAMETER : uint32 {
	PER_SCENE_DATA							= 0,
	CASCADE_SHADOW_MAPS						= 1,
	SHADOW_MAPS								= 2,
	G_BUFFER								= 3,
	HDR_RESULT								= 4,
	TONE_MAPPING_DATA						= 5,
	PER_PASS_DATA							= 6,
	PER_INSTANCE_DATA						= 7,
	LIGHT_CAMERA_DATA						= 8,
	WORLD_TRANSFORM_DATA					= 9,
	BONE_TRANSFORM							= 10,
	TERRAIN_LAYER							= 11,
	TERRAIN_COMPONENT_AND_WEIGHTMAP			= 12,
	WORLE_TRANSFORM_INDEX					= 13,
	SPRITE_DATA								= 14,
};

struct GBuffer {
	const static uint32 g_unNumGBuffers = 3;

	std::array<TextureRef<RenderTargetTexture>, GBuffer::g_unNumGBuffers> GBuffers;

	void Initialize(uint32 nPendingFrameIndex);

};

constexpr UINT DESCRIPTOR_PER_DRAW = 100'000;

struct IMesh;

class RenderManager {

	DECLARE_SINGLE(RenderManager)

public:
	constexpr static uint32 g_unMaxPendingFrames = 3;
	constexpr static uint32 g_unMaxSpriteLayers = 3;

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice);
	void BuildRenderGraph();
	void Render();
	void ShowDebugOptions();

	template<typename T> requires std::derived_from<T, IGameObject>
	void Add(std::shared_ptr<T> pObj);
	void Reset();

	// Sprites
	void AddSprite(const TextureRef<Texture>& texHandle, const SpriteRect& rect, uint32 unLayer);
	void AddSprite(const TextureRef<RenderTargetTexture>& texHandle, const SpriteRect& rect, uint32 unLayer);
	void AddSprite(const TextureRef<DepthStencilTexture>& texHandle, const SpriteRect& rect, uint32 unLayer);
	void AddSprite(const TextureRef<UnorderedAccessTexture>& texHandle, const SpriteRect& rect, uint32 unLayer);

	const std::shared_ptr<IMesh> GetQuadMesh() const { return m_pQuadMesh; }

public:
	DescriptorHeap& GetDescriptorHeap() { return m_DescriptorHeapForDraw[m_unCurrentContextIndex]; }

	template<typename T>
	ConstantBuffer AllocCBuffer() {
		return m_ConstantBufferPool[m_unCurrentContextIndex].Allocate<T>();
	}

	template<typename T>
	StructuredBuffer AllocSBuffer(uint32 unElementCount) {
		if (unElementCount == 0) {
			return m_StructuredBufferPool[m_unCurrentContextIndex].Allocate<T>(1);	// Temporary
		}
		return m_StructuredBufferPool[m_unCurrentContextIndex].Allocate<T>(unElementCount);
	}

public:
	// Renderable Items Getter
	const auto& GetObjectsToRender() const { return m_pObjectsToRender; }
	const auto& GetTransparentObjectsToRender() const { return m_pTransparentObjectsToRender; }
	const auto& GetSprites() const { return m_pSpritesToRender; }

private:
	void BindPerSceneData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle);

public:
	//std::vector<std::shared_ptr<RenderPass>> m_pRenderPasses = {};
	//std::shared_ptr<ForwardPass> m_pForwardPass;
	RenderGraph m_RenderGraph;

	ComPtr<ID3D12Device> m_pd3dDevice; // ref of D3DCore::m_pd3dDevice
	
	// Mesh
	static ComPtr<ID3D12RootSignature> g_pd3dGlobalRootSignature;

private:
	// Objects Ready-To-Draw
	std::vector<std::shared_ptr<IGameObject>> m_pObjectsToRender;
	std::vector<std::shared_ptr<IGameObject>> m_pTransparentObjectsToRender;

	std::shared_ptr<IMesh> m_pQuadMesh;

private:
	// Sprite
	struct SpriteRenderParameter {
		const TextureHandle& texHandle;
		SpriteRect Rect;

		SpriteRenderParameter(const TextureRef<Texture>& texHandle, const SpriteRect& r) : texHandle{ texHandle.srvHandle }, Rect{ r } {}
		SpriteRenderParameter(const TextureRef<RenderTargetTexture>& texHandle, const SpriteRect& r) : texHandle{ texHandle.srvHandle }, Rect{ r } {}
		SpriteRenderParameter(const TextureRef<DepthStencilTexture>& texHandle, const SpriteRect& r) : texHandle{ texHandle.srvHandle }, Rect{ r } {}
		SpriteRenderParameter(const TextureRef<UnorderedAccessTexture>& texHandle, const SpriteRect& r) : texHandle{ texHandle.srvHandle }, Rect{ r } {}
	};

	std::array<std::vector<SpriteRenderParameter>, g_unMaxSpriteLayers> m_pSpritesToRender;

private:
	// Frame Resources
	DescriptorHeap			m_DescriptorHeapForDraw[g_unMaxPendingFrames];
	ConstantBufferPool		m_ConstantBufferPool[g_unMaxPendingFrames];
	StructuredBufferPool	m_StructuredBufferPool[g_unMaxPendingFrames];

	GBuffer									m_GBuffers[g_unMaxPendingFrames];
	TextureRef<RenderTargetTexture>			m_HDRRenderTargetIDs[g_unMaxPendingFrames];
	TextureRef<RenderTargetTexture>			m_LDRRenderTargetIDs[g_unMaxPendingFrames];



#pragma region D3D
public:
	uint32 GetCurrentContextIndex() const { return m_unCurrentContextIndex; }

	const TextureRef<RenderTargetTexture>& GetCurrentBackBuffer() const;
	const TextureRef<DepthStencilTexture>& GetDepthStencilBuffer() const;
	
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle() const;
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilBufferHandle() const;

	const GBuffer& GetCurrentGBuffer() const;

	const TextureRef<RenderTargetTexture>& GetCurrentHDRBuffer() const;
	const TextureRef<RenderTargetTexture>& GetCurrentLDRBuffer() const;

	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentHDRBufferHandle() const;
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentLDRBufferHandle() const;

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_ppd3dCommandList[m_unCurrentContextIndex]; }

	void WaitForGPUComplete();

private:
	void OnPrepareRender();
	void OnPostRender();

private:
	void CreateFence();
	void CreateCommandQueueAndList();
	void CreateSwapChain();
	void CreateRenderTarget();
	void CreateDepthStencil();

private:
	void Present();

private:
	uint64 Fence();
	void WaitForFenceValue(uint64 un64ExpectedFenceValue);
	void ChangeSwapChainState();

private:
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain = nullptr;
	const DXGI_FORMAT m_dxgiRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	uint32 m_unBackBufferIndex = 0;

	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue								= nullptr;
	ComPtr<ID3D12CommandAllocator>		m_ppd3dCommandAllocator[g_unMaxPendingFrames]	= {};
	ComPtr<ID3D12GraphicsCommandList>	m_ppd3dCommandList[g_unMaxPendingFrames]		= {};

	ComPtr<ID3D12Fence> m_pd3dFence							= nullptr;
	HANDLE m_hFenceEvent									= nullptr;
	uint64 m_un64LastFenceValues[g_unMaxPendingFrames]		= {};
	uint64 m_un64FenceValues								= 0;

	uint32 m_unCurrentContextIndex = 0;

	// SRV - RTV/DSV
	TextureRef<RenderTargetTexture> m_BackBufferIDs[g_unMaxPendingFrames];
	TextureRef<DepthStencilTexture> m_DepthStencilID;

#pragma endregion D3D
};

template<typename T> requires std::derived_from<T, IGameObject>
inline void RenderManager::Add(std::shared_ptr<T> pObj)
{
	auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();
	auto pBaseColorTex = pMeshRenderer->GetMaterialHandle(0).GetResource()->GetTexture(0);


	(pBaseColorTex->GetAlphaMode() == Texture::ALPHA_MODE::Transparent)
		? m_pTransparentObjectsToRender.push_back(pObj)
	    : m_pObjectsToRender.push_back(pObj);
}
