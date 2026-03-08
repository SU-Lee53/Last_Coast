#pragma once

class TerrainPass : public IRenderPass {
public:
	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const RenderPassInput& input, 
		OUT RenderPassOutput& output, 
		OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override;

	virtual void OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const override;

};
