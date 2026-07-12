#pragma once
#include "RenderPass.h"

class NavMeshDebugPass : public IRenderPass {
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
	void CreatePipelineStates();
	ComPtr<ID3D12PipelineState> CreateLinePipelineState(const std::string& strPSName);
	void BuildNavMeshLineVertices();
	void DrawLines(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ID3D12PipelineState* pd3dPipelineState, const std::vector<Vector3>& v3Vertices);

private:
	ComPtr<ID3D12PipelineState> m_pd3dPolygonEdgePSO = nullptr;
	ComPtr<ID3D12PipelineState> m_pd3dAdjacencyPSO = nullptr;

	std::vector<Vector3> m_v3PolygonEdgeVertices;
	std::vector<Vector3> m_v3AdjacencyVertices;

	bool m_bEnabled = false;
	bool m_bShowPolygonEdges = true;
	bool m_bShowAdjacency = false;
	bool m_bBuilt = false;
	float m_fHeightOffset = 5.0f;	// cm, prevents z-fighting with ground geometry
};
