#pragma once
#include "DescriptorHeap.h"
#include "RenderPass.h"
#include "MeshRenderer.h"

// 2월에 전부 갈아엎을 예정

enum class ROOT_PARAMETER : uint32 {
	SCENE_CAM_DATA					= 0,
	SCENE_LIGHT_DATA				= 1,
	SCENE_SKYBOX					= 2,
	SCENE_TERRAIN_DATA				= 3,
	SCENE_TERRAIN_COMPONENT_DATA	= 4,
	PASS_INSTANCE_DATA				= 5,
	OBJ_MATERIAL_DATA				= 6,
	OBJ_BONE_TRANSFORM_DATA			= 7,
	OBJ_TEXTURES					= 8,
};

constexpr UINT DESCRIPTOR_PER_DRAW = 1000000;

struct MeshRenderParameters {
	Matrix mtxWorld;
};

struct InstancePair {
	std::shared_ptr<IMeshRenderer> meshRenderer;
	std::vector<MeshRenderParameters> InstanceDatas;
};

class RenderManager {

	DECLARE_SINGLE(RenderManager)

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void CreateSkyboxPipelineState(ComPtr<ID3D12Device> pd3dDevice);
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

private:
	void RenderAnimated(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descriptorHandleFromPassStart);
	void RenderSkybox(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descriptorHandleFromPassStart);

public:
	template<typename T> requires std::derived_from<T, IMeshRenderer>
	void Add(std::shared_ptr<T> pRenderItem, MeshRenderParameters renderParam);
	void AddAnimatedObject(std::shared_ptr<GameObject> pObj);
	void Clear();

public:
	DescriptorHeap& GetDescriptorHeap() { return m_DescriptorHeapForDraw; }

private:
	//std::vector<std::shared_ptr<RenderPass>> m_pRenderPasses = {};
	std::shared_ptr<ForwardPass> m_pForwardPass;


public:
	ComPtr<ID3D12Device> m_pd3dDevice; // ref of D3DCore::m_pd3dDevice
	
	// Mesh
	static ComPtr<ID3D12RootSignature> g_pd3dGlobalRootSignature;
	DescriptorHeap m_DescriptorHeapForDraw;
	
	// Skybox
	ComPtr<ID3D12PipelineState> m_pd3dSkyboxPipelineState;

	// Pass 별 분리 필요 ( Forward / Differed )
	// 방법은 더 연구할 것
	std::array<std::unordered_map<uint64_t, UINT>, OBJECT_RENDER_TYPE_COUNT> m_InstanceIndexMap;
	std::array<std::vector<InstancePair>, 2> m_InstanceDatas;

	// 키프레임 애니메이션 GameObjects (인스턴싱 불가)
	std::vector<std::shared_ptr<GameObject>> m_pAnimatedObjects;

	UINT m_nInstanceIndex[2] = {0, 0};
};

template<typename T> requires std::derived_from<T, IMeshRenderer>
inline void RenderManager::Add(std::shared_ptr<T> pRenderItem, MeshRenderParameters renderParam)
{
	const uint64_t& key = pRenderItem->GetID();
	UINT nRenderType = pRenderItem->GetRenderType();

	auto it = m_InstanceIndexMap[nRenderType].find(key);
	if (it == m_InstanceIndexMap[nRenderType].end()) {
		InstancePair instancePair{ pRenderItem, std::vector<MeshRenderParameters>{ renderParam } };

		m_InstanceIndexMap[nRenderType][key] = m_nInstanceIndex[nRenderType]++;
		m_InstanceDatas[nRenderType].push_back(instancePair);
	}
	else {
		m_InstanceDatas[nRenderType][it->second].InstanceDatas.emplace_back(renderParam);
	}
}
