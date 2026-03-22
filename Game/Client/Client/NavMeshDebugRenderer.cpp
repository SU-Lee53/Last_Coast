#include "pch.h"
#include "NavMeshDebugRenderer.h"

NavMeshDebugRenderer::NavMeshDebugRenderer()
{
}

NavMeshDebugRenderer::~NavMeshDebugRenderer()
{
}

void NavMeshDebugRenderer::Initialize(std::shared_ptr<INavMesh> pNavMesh)
{
	if (!pNavMesh) {
		return;
	}

	BuildLineBuffersFromNavMesh(pNavMesh);
	m_bInitialized = true;
}

void NavMeshDebugRenderer::BuildLineBuffersFromNavMesh(std::shared_ptr<INavMesh> pNavMesh)
{
	// INavMesh의 GetDebugLineVertices()를 사용하여 디버그 데이터 가져오기
	NavMeshDebugData debugData = pNavMesh->GetDebugLineVertices();

	// 1. 폴리곤 외곽선 VertexBuffer 생성
	if (!debugData.PolygonEdges.empty()) {
		m_nPolygonEdgeVertices = static_cast<uint32>(debugData.PolygonEdges.size());
		m_PolygonEdgeVertexBuffer = RESOURCE->CreateVertexBuffer(debugData.PolygonEdges, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
	}

	// 2. 인접성 그래프 VertexBuffer 생성
	if (!debugData.AdjacencyEdges.empty()) {
		m_nAdjacencyEdgeVertices = static_cast<uint32>(debugData.AdjacencyEdges.size());
		m_AdjacencyEdgeVertexBuffer = RESOURCE->CreateVertexBuffer(debugData.AdjacencyEdges, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
	}
}

void NavMeshDebugRenderer::RenderPolygonEdges(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (!m_bInitialized || !m_bEnabled || m_nPolygonEdgeVertices == 0) {
		return;
	}

	// 프리미티브 토폴로지 설정 (라인 리스트)
	pd3dCommandList->IASetPrimitiveTopology(m_Topology);

	// 버텍스 버퍼 바인딩
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_PolygonEdgeVertexBuffer.VertexBufferView);

	// 폴리곤 외곽선 그리기
	pd3dCommandList->DrawInstanced(m_nPolygonEdgeVertices, 1, 0, 0);
}

void NavMeshDebugRenderer::RenderAdjacencyEdges(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (!m_bInitialized || !m_bEnabled || m_nAdjacencyEdgeVertices == 0) {
		return;
	}

	// 프리미티브 토폴로지 설정 (라인 리스트)
	pd3dCommandList->IASetPrimitiveTopology(m_Topology);

	// 버텍스 버퍼 바인딩
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_AdjacencyEdgeVertexBuffer.VertexBufferView);

	// 인접성 그래프 그리기
	pd3dCommandList->DrawInstanced(m_nAdjacencyEdgeVertices, 1, 0, 0);
}

void NavMeshDebugRenderer::UpdatePathNodes(const std::vector<Vector3>& polyNodeCenters)
{
	m_nPathNodeVertices = 0;

	// 노드가 2개 미만이면 그릴 선이 없다
	if (polyNodeCenters.size() < 2) {
		return;
	}

	// 폴리곤 중심점 목록 → LINELIST 형식으로 변환
	// [c0,c1, c1,c2, c2,c3, ...]
	std::vector<Vector3> lineVertices;
	lineVertices.reserve((polyNodeCenters.size() - 1) * 2);

	for (size_t i = 0; i + 1 < polyNodeCenters.size(); ++i)
	{
		lineVertices.push_back(polyNodeCenters[i]);
		lineVertices.push_back(polyNodeCenters[i + 1]);
	}

	m_nPathNodeVertices = static_cast<uint32>(lineVertices.size());
	m_PathNodeVertexBuffer = RESOURCE->CreateVertexBuffer(lineVertices, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
}

void NavMeshDebugRenderer::RenderPathNodes(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (!m_bEnabled || m_nPathNodeVertices == 0) {
		return;
	}

	pd3dCommandList->IASetPrimitiveTopology(m_Topology);
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_PathNodeVertexBuffer.VertexBufferView);
	pd3dCommandList->DrawInstanced(m_nPathNodeVertices, 1, 0, 0);
}
