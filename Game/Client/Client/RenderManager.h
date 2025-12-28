#pragma once
#include "DescriptorHeap.h"
#include "RenderPass.h"
#include "MeshRenderer.h"

/*
	2025.10.08
	- 대대적인 개편 필요
		- RenderManager -> RenderPass -> Pipeline 의 구조는 매우 복잡
		- RenderManager 에 Object 를 모아두고 MeshRenderer를 이용하여 렌더링
		- RenderPass 는 남겨놓고, 렌더링 단위를 변경하는 의미로 넘어가야 할듯

		+ Global 로 사용할 Root Signature RenderManager 에서 관리하도록 하는것도 괜찮아보임
*/

#define ROOT_PARAMETER_SCENE_CAM_DATA		0
#define ROOT_PARAMETER_SCENE_LIGHT_DATA		1
#define ROOT_PARAMETER_SCENE_SKYBOX			2
#define ROOT_PARAMETER_PASS_INSTANCE_DATA	3
#define ROOT_PARAMETER_OBJ_MATERIAL_DATA	4
#define ROOT_PARAMETER_OBJ_TEXTURES			5
#define ROOT_PARAMETER_PLAYER_DATA			6

constexpr UINT DESCRIPTOR_PER_DRAW = 1000000;

enum OBJECT_RENDER_TYPE : UINT {
	OBJECT_RENDER_FORWARD = 0,
	OBJECT_RENDER_DIFFERED,

	OBJECT_RENDER_TYPE_COUNT
};

struct MeshRenderParameters {
	Matrix mtxWorld;
};


struct InstancePair {
	std::shared_ptr<MeshRenderer> meshRenderer;
	std::vector<MeshRenderParameters> InstanceDatas;
};

class RenderManager {
public:
	RenderManager(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	~RenderManager();

	void CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	
	void CreateSkyboxPipelineState(ComPtr<ID3D12Device> pd3dDevice);

	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

private:
	void RenderSkybox(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descriptorHandleFromPassStart);

public:
	template<typename T> requires std::derived_from<T, MeshRenderer> || std::same_as<T, Texture>
	void Add(std::shared_ptr<T> pRenderItem, MeshRenderParameters renderParam);
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

	// Sprite
	ComPtr<ID3D12RootSignature> m_pSpriteRootSignature;
	ComPtr<ID3D12PipelineState> m_pd3dSpritePipelineState;
	DescriptorHeap m_DescriptorHeapForSprite;

	// Skybox
	ComPtr<ID3D12PipelineState> m_pd3dSkyboxPipelineState;

	// Pass 별 분리 필요 ( Forward / Differed )
	// 방법은 더 연구할 것
	std::array<std::unordered_map<MeshRenderer, UINT>, OBJECT_RENDER_TYPE_COUNT> m_InstanceIndexMap;
	std::array<std::vector<InstancePair>, 2> m_InstanceDatas;

	UINT m_nInstanceIndex[2] = {0, 0};


};

template<typename T> requires std::derived_from<T, MeshRenderer> || std::same_as<T, Texture>
inline void RenderManager::Add(std::shared_ptr<T> pRenderItem, MeshRenderParameters renderParam)
{
	const MeshRenderer& key = *pRenderItem;
	UINT nRenderType = pRenderItem->m_eObjectRenderType;

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
