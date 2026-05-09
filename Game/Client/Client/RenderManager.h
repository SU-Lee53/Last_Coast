#pragma once
#include "DescriptorHeap.h"

#include "ConstantBufferPool.h"
#include "StructuredBufferPool.h"

#include "TextRenderer.h"
#include "RenderGraph.h"

enum class ROOT_PARAMETER : uint32 {
	PER_SCENE_DATA							= 0,
	CASCADE_SHADOW_MAPS						= 1,
	SHADOW_MAPS								= 2,
	G_BUFFER								= 3,
	HDR_RESULT								= 4,
	TONE_MAPPING_DATA						= 5,
	FOG_DATA								= 6,
	PER_PASS_BUFFERS						= 7,
	PER_PASS_TEXTURES						= 8,
	PER_INSTANCE_DATA						= 9,
	BONE_TRANSFORM_OFFSETS					= 10,
	LIGHT_CAMERA_DATA						= 11,
	TERRAIN_LAYER							= 12,
	TERRAIN_COMPONENT_AND_WEIGHTMAP			= 13,
	TERRAIN_WORLD_TRANSFORM					= 14,
	UI_DATA									= 15,
	PARTICLE_DATA							= 16,
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

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice);
	void BuildRenderGraph();
	void Render();
	void ShowDebugOptions();

	template<typename T> requires std::derived_from<T, IGameObject>
	void Add(std::shared_ptr<T> pObj);
	void Reset();

	const std::shared_ptr<IMesh> GetQuadMesh() const { return m_pQuadMesh; }
	TextRenderer& GetTextRenderer() { return m_TextRenderer; }

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

private:
	void BindPerSceneData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle);

public:
	RenderGraph m_RenderGraph;

	
	// Mesh
	static ComPtr<ID3D12RootSignature> g_pd3dGlobalRootSignature;
	
	// TextRenderer
	TextRenderer m_TextRenderer;



private:
	// Objects Ready-To-Draw
	std::vector<std::shared_ptr<IGameObject>> m_pObjectsToRender;
	std::vector<std::shared_ptr<IGameObject>> m_pTransparentObjectsToRender;

	std::shared_ptr<IMesh> m_pQuadMesh;

private:
	// Frame Resources
	DescriptorHeap			m_DescriptorHeapForDraw[g_unMaxPendingFrames];
	ConstantBufferPool		m_ConstantBufferPool[g_unMaxPendingFrames];
	StructuredBufferPool	m_StructuredBufferPool[g_unMaxPendingFrames];

	GBuffer									m_GBuffers[g_unMaxPendingFrames];
	TextureRef<RenderTargetTexture>			m_HDRRenderTargetIDs[2][g_unMaxPendingFrames];
	TextureRef<RenderTargetTexture>			m_LDRRenderTargetIDs[g_unMaxPendingFrames];


#pragma region D3D
public:
	uint32 GetCurrentContextIndex() const { return m_unCurrentContextIndex; }

	const TextureRef<RenderTargetTexture>& GetCurrentBackBuffer() const;
	const TextureRef<DepthStencilTexture>& GetDepthStencilBuffer() const;
	
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle() const;
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilBufferHandle() const;

	const GBuffer& GetCurrentGBuffer() const;

	const TextureRef<RenderTargetTexture>& GetCurrentHDRBuffer(int nIndex) const;
	const TextureRef<RenderTargetTexture>& GetCurrentLDRBuffer() const;

	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentHDRBufferHandle(int nIndex) const;
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentLDRBufferHandle() const;

	const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const { return m_ppd3dCommandList[m_unCurrentContextIndex]; }
	const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const { return m_pd3dCommandQueue; }

	void WaitForGPUComplete();

	void ImmediateStateTransition(
		const ComPtr<ID3D12Resource>& pResource,
		OUT D3D12_RESOURCE_STATES& outd3dState,
		D3D12_RESOURCE_STATES d3dTargetState
	);


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
	ComPtr<ID3D12Device> m_pd3dDevice; // ref of D3DCore::m_pd3dDevice
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain = nullptr;
	const DXGI_FORMAT m_dxgiRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	uint32 m_unBackBufferIndex = 0;

	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator>		m_ppd3dCommandAllocator[g_unMaxPendingFrames] = {};
	ComPtr<ID3D12GraphicsCommandList>	m_ppd3dCommandList[g_unMaxPendingFrames] = {};

	CommandListAllocator m_ImmediateTransitionCmdLists;


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

	if (!pBaseColorTex) {
		std::string strErr = std::format("{} - {} : No BaseColor", __FILE__, __LINE__);
		//OutputDebugStringA(strErr.c_str());
		return;
	}
	(pBaseColorTex->GetAlphaMode() == Texture::ALPHA_MODE::Transparent)
		? m_pTransparentObjectsToRender.push_back(pObj)
	    : m_pObjectsToRender.push_back(pObj);
}
