#pragma once
#include "RenderPass.h"

class TransparentForwardLightingPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData;
		uint32 sbWorldTransformIndex;
		std::shared_ptr<AnimationController> pAnimationController;
	};

	using RenderQueue = std::vector<std::pair<std::shared_ptr<IMesh>, TransparentForwardLightingPass::RenderParameter>>;

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
	void BindGeometryData(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const std::vector<std::shared_ptr<IGameObject>>& frustumCulled,
		OUT DescriptorHandle& outDescHandle) const;

	void CreatePipelineState();

private:
	mutable RenderQueue m_RenderQueueCached;

	ComPtr<ID3D12PipelineState> m_pd3dStandardPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dAnimatedPipelineState;

};

