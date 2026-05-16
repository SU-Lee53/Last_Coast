#pragma once
#include "RenderPass.h"

class BloomPass : public IRenderPass {
private:
	constexpr static uint32 BLOOM_EXTRACT_THREAD_X = 8;
	constexpr static uint32 BLOOM_EXTRACT_THREAD_Y = 8;
	
	constexpr static uint32 BLOOM_BLUR_THREAD_X = 16;
	constexpr static uint32 BLOOM_BLUR_THREAD_Y = 16;

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
	void CreateRootSignature();
	void CreatePipelineStates();

private:
	ComPtr<ID3D12PipelineState> m_pd3dBrightExtractPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dBlurHPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dBlurVPipelineState;
};

