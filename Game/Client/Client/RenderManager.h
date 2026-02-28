#pragma once
#include "DescriptorHeap.h"
#include "RenderPass.h"
#include "MeshRenderer.h"

#include "ConstantBufferPool.h"
#include "StructuredBufferPool.h"

#include "RenderGraph.h"

// 2월에 전부 갈아엎을 예정

enum class ROOT_PARAMETER : uint32 {
	PER_SCENE_DATA							= 0,
	PER_PASS_DATA							= 1,
	PER_INSTANCE_DATA						= 2,
	WORLD_TRANSFORM_DATA					= 3,
	BONE_TRANSFORM							= 4,
	TERRAIN_LAYER							= 5,
	TERRAIN_COMPONENT_AND_WEIGHTMAP			= 6,
};

constexpr UINT DESCRIPTOR_PER_DRAW = 100'000;

class RenderManager {

	DECLARE_SINGLE(RenderManager)

public:
	constexpr static uint32 m_unSwapChainBuffers = 2;

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice);
	void BuildRenderGraph();
	void Render();

	template<typename T> requires std::derived_from<T, IGameObject>
	void Add(std::shared_ptr<T> pObj);
	void Clear();

public:
	DescriptorHeap& GetDescriptorHeap() { return m_DescriptorHeapForDraw; }

	template<typename T>
	ConstantBuffer AllocCBuffer() {
		return m_ConstantBufferPool.Allocate<T>();
	}

	void ResetCBufferBool() {
		m_ConstantBufferPool.Reset();
	}

	template<typename T>
	StructuredBuffer AllocSBuffer(uint32 unElementCount) {
		if (unElementCount == 0) {
			return m_StructuredBufferPool.Allocate<T>(1);	// Temporary

		}
		return m_StructuredBufferPool.Allocate<T>(unElementCount);
	}

	void ResetSBufferBool() {
		m_StructuredBufferPool.Reset();
	}

	void FrustumCulling(const BoundingFrustum& xmFrustumWorld);
private:
	void BindPerSceneData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle);

private:
	//std::vector<std::shared_ptr<RenderPass>> m_pRenderPasses = {};
	//std::shared_ptr<ForwardPass> m_pForwardPass;
	RenderGraph m_RenderGraph;


public:
	ComPtr<ID3D12Device> m_pd3dDevice; // ref of D3DCore::m_pd3dDevice
	
	// Mesh
	static ComPtr<ID3D12RootSignature> g_pd3dGlobalRootSignature;
	DescriptorHeap m_DescriptorHeapForDraw;
	
	// Objects Ready-To-Draw
	std::vector<std::shared_ptr<IGameObject>> m_pRenderItems;

	// Cache of Current Draw
	std::vector<std::shared_ptr<IGameObject>> m_pFrustumCulledObjects;

private:
	ConstantBufferPool		m_ConstantBufferPool;
	StructuredBufferPool	m_StructuredBufferPool;

#pragma region D3D
public:
	const std::shared_ptr<RenderTargetTexture> GetCurrentBackBuffer() const;
	const std::shared_ptr<DepthStencilTexture> GetDepthStencilBuffer() const;
	
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferHandle() const;
	const CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilBufferHandle() const;

	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_pd3dCommandList; }

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
	void MoveToNextFrame();

private:
	uint64 Fence();
	void ChangeSwapChainState();

private:
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain = nullptr;
	const DXGI_FORMAT m_dxgiRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	uint32 m_unSwapChainBufferIndex = 0;

	ComPtr<ID3D12CommandAllocator>		m_pd3dCommandAllocator	= nullptr;
	ComPtr<ID3D12GraphicsCommandList>	m_pd3dCommandList		= nullptr;
	ComPtr<ID3D12CommandQueue>			m_pd3dCommandQueue		= nullptr;

	ComPtr<ID3D12Fence> m_pd3dFence				= nullptr;
	HANDLE m_hFenceEvent						= nullptr;
	uint64 m_nFenceValues[m_unSwapChainBuffers];

	// SRV - RTV/DSV
	std::pair<Texture::ID, Texture::ID> m_BackBufferIDs[m_unSwapChainBuffers];
	std::pair<Texture::ID, Texture::ID> m_DepthStencilID;

#pragma endregion D3D
};

template<typename T> requires std::derived_from<T, IGameObject>
inline void RenderManager::Add(std::shared_ptr<T> pObj)
{
	m_pRenderItems.push_back(pObj);
}
