#pragma once
#include "RenderPass.h"
class AutoExposurePass : public IRenderPass {
private:
	constexpr static uint32 THREAD_X = 16;
	constexpr static uint32 THREAD_Y = 16;

	constexpr static uint32 FINAL_THREAD_X = 256;
	constexpr static uint32 FINAL_THREAD_Y = 0;

	struct CB_AUTO_EXPOSURE_INOUT_DATA {
		XMINT2 xmi2InputSize;
		XMINT2 xmi2OutputSize;
	};

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

private:
	ComPtr<ID3D12PipelineState> m_pd3dLuminanceExtractPipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dReduceLuminancePipelineState;
	ComPtr<ID3D12PipelineState> m_pd3dFinalPipelineState;
};

