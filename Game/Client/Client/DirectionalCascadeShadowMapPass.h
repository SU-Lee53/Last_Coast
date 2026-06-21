#pragma once
#include "RenderPass.h"

class DirectionalCascadeShadowMapPass : public IRenderPass {
public:
	constexpr static uint32 g_unNumCascade = 4;
	constexpr static uint32 g_unCascadeShadowMapSize[g_unNumCascade] = {
		2048, 2048, 1024, 512
		//4096, 2048, 1024, 512
	};

	constexpr static float g_fMaxShadowDistance = 100_m;
	constexpr static float g_fLightDistanceMargin = 5_m;
	constexpr static float g_fLambda = 0.7f; // 0 : Uniform / 1 : log scale

public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

private:
	void CreatePipelineState();

	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unCascade) const;

	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<IGameObject*>& frustumCulled,
		OUT DescriptorHandle& outDescHandle);

	void DrawGeometry(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle);

	void DrawTerrain(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<TerrainComponent*>& frustumCulled,
		OUT DescriptorHandle& outDescHandle);

	void ComputeCascade();

	virtual void ShowDebugInfo() override;

private:
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData{};
		int32 nInstances = 0;
		uint32 unBoneOffsetStart = 0;
		uint32 unBoneOffsetCount = 0;
		ID3D12PipelineState* pd3dPipelineState = nullptr;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, DirectionalCascadeShadowMapPass::RenderParameter>>;

	struct CascadeCameraData {
		BoundingFrustum xmShadowFrustum;	// Real shadow map projection area
		BoundingFrustum xmCasterCullFrustum;	// Wider area for caster collecting

		Matrix mtxLightViewProj;
		Matrix mtxToShadowMap;
	};

	std::array<CascadeCameraData, g_unNumCascade> m_CascadeCached;
	RenderQueue m_RenderQueueCached;
	std::vector<IGameObject*> m_FrustumCulledCached;

	ComPtr<ID3D12PipelineState> m_pd3dStandardPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dAnimatedPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dTerrainPipelineState;

	TextureRef<DepthStencilTexture> m_ShadowMapRefs[g_unNumCascade];

	struct CachedData {
		IndexMap<MeshRenderer::ID, std::pair<const MeshRenderer*, std::vector<const IGameObject*>>> frustumCulledMap;

		std::vector<WorldTransformData> sbWorldTransformDatas;
		std::vector<Matrix> sbBoneTransformDatas;
		std::vector<int32> nBoneOffsets;
		D3D12_GPU_VIRTUAL_ADDRESS d3dBoneOffsetGPUAddress = 0;

		struct AnimationInstancingData {
			int32 unOffset = 0;
		};

		std::unordered_map<const AnimationController*, AnimationInstancingData> animationInstancingData;

		void Clear() {
			frustumCulledMap.Clear();

			sbWorldTransformDatas.clear();
			sbBoneTransformDatas.clear();
			nBoneOffsets.clear();
			d3dBoneOffsetGPUAddress = 0;

			animationInstancingData.clear();
		}

	} m_CachedData;

	// Shadow map interval update
	std::array<bool, g_unNumCascade> m_bNeedUpdates{};
	std::array<float, g_unNumCascade> m_fCascadeUpdateTimers{};
	constexpr static std::array<float, g_unNumCascade> g_fCascadeUpdateFPS = {
		60.0f, 45.0f, 30.0f, 15.0f
	};

	bool m_bFirstUpdate = true;
};

