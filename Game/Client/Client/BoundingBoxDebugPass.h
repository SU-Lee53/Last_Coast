#pragma once
#include "RenderPass.h"

class BoundingBoxDebugPass : public IRenderPass {
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

	virtual void ShowDebugInfo() override;

private:
	void CreatePipelineState();
	void AppendBoundingBoxLines(const BoundingBox& xmAABB);
	void AppendBoundingBoxLines(const BoundingOrientedBox& xmOBB);
	void BuildCollisionLineVertices();

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;
	std::vector<Vector3> m_v3LineVertices;
	bool m_bEnabled = false;
	uint32 m_unDrawnColliders = 0;
};
