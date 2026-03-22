#pragma once
#include "ShaderResource.h"

class NavMeshDebugRenderer
{
public:
	NavMeshDebugRenderer();
	~NavMeshDebugRenderer();

	// NavMesh 데이터를 받아서 렌더링 준비
	void Initialize(std::shared_ptr<INavMesh> pNavMesh);

	// 렌더링
	void RenderPolygonEdges(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	void RenderAdjacencyEdges(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	// A* 경로 노드 업데이트 및 렌더링 (파란색)
	// polyNodeCenters : PathDebugInfo::polyNodeCenters (폴리곤 무게중심 목록)
	void UpdatePathNodes(const std::vector<Vector3>& polyNodeCenters);
	void RenderPathNodes(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	// 디버그 렌더링 활성화/비활성화
	void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }
	bool IsEnabled() const { return m_bEnabled; }

private:
	void BuildLineBuffersFromNavMesh(std::shared_ptr<INavMesh> pNavMesh);

private:
	// 폴리곤 외곽선 (녹색)
	VertexBuffer m_PolygonEdgeVertexBuffer;
	uint32 m_nPolygonEdgeVertices = 0;

	// 인접성 그래프 (빨간색)
	VertexBuffer m_AdjacencyEdgeVertexBuffer;
	uint32 m_nAdjacencyEdgeVertices = 0;

	// A* 경로 노드 연결선 (파란색)
	VertexBuffer m_PathNodeVertexBuffer;
	uint32 m_nPathNodeVertices = 0;

	D3D12_PRIMITIVE_TOPOLOGY m_Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	bool m_bEnabled = true;
	bool m_bInitialized = false;
};
