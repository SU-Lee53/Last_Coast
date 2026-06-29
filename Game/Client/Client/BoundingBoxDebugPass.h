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
	void AppendNavMeshLines();   // NavMesh 폴리곤 외곽선을 라인 목록에 추가 (정렬/커버리지 확인용)

private:
	ComPtr<ID3D12PipelineState> m_pd3dPipelineState = nullptr;
	std::vector<Vector3> m_v3LineVertices;
	bool m_bEnabled = false;
	uint32 m_unDrawnColliders = 0;

	// NavMesh 디버그 오버레이 (월드와 정렬/커버리지 확인용). 기본 ON.
	bool m_bDrawNavMesh = true;
	bool m_bNavMeshBuilt = false;
	std::vector<Vector3> m_v3NavMeshLines;  // 캐시된 폴리곤 외곽선 (LINELIST)
};
