#pragma once
#include "RenderPass.h"

class ForwardPass : public IRenderPass {
	struct RenderParameter {
		CB_INSTANCE_DATA cbInstanceData;
		std::vector<Matrix> sbWorldTransformData;
		std::vector<AnimationController*> pAnimationControllers;
	};

	using RenderQueue = std::vector<std::pair<IMesh*, ForwardPass::RenderParameter>>;

public:
	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const RenderPassInput& input,
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPostRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const RenderPassInput& input, 
		OUT DescriptorHandle& outDescHandle) const override;

private:
	mutable RenderQueue m_RenderQueueCached;

};
