#pragma once

class DeferredLightingPass : public IRenderPass {
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

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

};


