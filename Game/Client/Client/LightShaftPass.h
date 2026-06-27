#pragma once
#include "RenderPass.h"

class LightShaftPass : public IRenderPass {
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

	virtual void ShowDebugInfo() override;

private:
	void CreatePipelineState();
	CB_LIGHT_SHAFT_DATA MakeLightShaftCBData();

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState;

	Vector2 m_v2LightScreenPosition = Vector2{ 0.5f, 0.5f };
	bool m_bLightInFront = false;
	bool m_bShouldRender = false;
};
