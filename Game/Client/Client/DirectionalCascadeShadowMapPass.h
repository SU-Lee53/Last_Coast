#pragma once
#include "RenderPass.h"

class DirectionalCascadeShadowMapPass : public IRenderPass {
public:
	constexpr static uint32 g_unNumCascade = 4;
	constexpr static uint32 g_unCascadeShadowMapSize[g_unNumCascade] = {
		2048, 1024, 512, 256
	};

public:
	virtual void Initialize() override;

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;


	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) const override;

private:
	void CreatePipelineState();

	void SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unCascade) const;

	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<std::shared_ptr<IGameObject>>& frustumCulled,
		OUT DescriptorHandle& outDescHandle) const;

	void DrawGeometry(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle) const;

	void DrawTerrain(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		OUT DescriptorHandle& outDescHandle) const;

	void ComputeCascade() const;

private:
	struct RenderParameter {
		std::vector<WorldTransformData> sbWorldTransformData;
		std::vector<AnimationController*> pAnimationControllers;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, DirectionalCascadeShadowMapPass::RenderParameter>>;

	struct CascadeCameraData {
		BoundingFrustum xmFrustum;
		Matrix mtxLightViewProj;
		Matrix mtxToShadowMap;
	};

	mutable std::array<CascadeCameraData, g_unNumCascade> m_CascadeCached;
	mutable RenderQueue m_RenderQueueCached;

	ComPtr<ID3D12PipelineState> m_pd3dStandardPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dAnimatedPipelineState;

	Texture::ID m_ShadowMapSRVID[RenderManager::g_unMaxPendingFrames][g_unNumCascade];
	Texture::ID m_ShadowMapDSVID[RenderManager::g_unMaxPendingFrames][g_unNumCascade];
};

