#pragma once
#include "RenderPass.h"

class SSAOPass : public IRenderPass {
public:
	virtual void Initialize() override;

	virtual void OnPreRender(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		const RenderPassInput& input,
		OUT RenderPassOutput& output,
		OUT DescriptorHandle& outDescHandle) override;

	virtual void Render(
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
	void CreatePipelineStates();
	void CreateNoiseTexture();

private:
	ComPtr<ID3D12PipelineState> m_pd3dSSAOPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dSSAOBlurPipelineState;

private:
	TextureRef<Texture> m_NoiseTexture;

};

