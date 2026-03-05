#include "pch.h"
#include "Mesh.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mesh 

IMesh::IMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
{
	m_d3dPrimitiveTopology = d3dTopology;
	m_nSlot = 0;
	m_nVertices = meshLoadInfo.v3Positions.size();
	m_nOffset = 0;

	m_Positions = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Positions, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));

	m_xmOBB.Center = meshLoadInfo.v3AABBCenter;
	m_xmOBB.Extents= meshLoadInfo.v3AABBExtents;

	// IB
	if (meshLoadInfo.unIndices.size() != 0) {
		m_IndexBuffer = RESOURCE->CreateIndexBuffer(meshLoadInfo.unIndices);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenMesh

FullScreenMesh::FullScreenMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: IMesh(meshLoadInfo, d3dTopology)
{
}

void FullScreenMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_Positions.VertexBufferView);

	if (m_IndexBuffer.nIndices != 0) {
		uint32 unIndices = (nIndexCount == -1) ? m_IndexBuffer.nIndices : nIndexCount;
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(unIndices, nInstanceCount, unStartIndex, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StaticMesh

StaticMesh::StaticMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: IMesh(meshLoadInfo, d3dTopology)
{
	m_Normals = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Normals, std::to_underlying(MESH_ELEMENT_TYPE::NORMAL));
	m_Tangents = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, std::to_underlying(MESH_ELEMENT_TYPE::TANGENT));
	m_TexCoords = RESOURCE->CreateVertexBuffer(meshLoadInfo.v2TexCoord0, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));
}

void StaticMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[4] = { 
		m_Positions.VertexBufferView,
		m_Normals.VertexBufferView,
		m_Tangents.VertexBufferView,
		m_TexCoords.VertexBufferView
	};
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffer.nIndices != 0) {
		uint32 unIndices = (nIndexCount == -1) ? m_IndexBuffer.nIndices : nIndexCount;
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(unIndices, nInstanceCount, unStartIndex, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinnedMesh

SkinnedMesh::SkinnedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: StaticMesh(meshLoadInfo, d3dTopology)
{
	m_BlendIndices = RESOURCE->CreateVertexBuffer(meshLoadInfo.xmun4BlendIndices, std::to_underlying(MESH_ELEMENT_TYPE::TANGENT));
	m_BlendWeights = RESOURCE->CreateVertexBuffer(meshLoadInfo.v4BlendWeights, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));
}

void SkinnedMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[6] = {
		m_Positions.VertexBufferView,
		m_Normals.VertexBufferView,
		m_Tangents.VertexBufferView,
		m_TexCoords.VertexBufferView,
		m_BlendIndices.VertexBufferView,
		m_BlendWeights.VertexBufferView
	};
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffer.nIndices != 0) {
		uint32 unIndices = (nIndexCount == -1) ? m_IndexBuffer.nIndices : nIndexCount;
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(unIndices, nInstanceCount, unStartIndex, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}

}

TerrainMesh::TerrainMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: IMesh(meshLoadInfo, d3dTopology)
{
	m_Normals = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Normals, std::to_underlying(MESH_ELEMENT_TYPE::NORMAL));
	m_Tangents = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, std::to_underlying(MESH_ELEMENT_TYPE::TANGENT));
}

void TerrainMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[3] = {
		m_Positions.VertexBufferView,
		m_Normals.VertexBufferView,
		m_Tangents.VertexBufferView,
	};
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffer.nIndices != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(nIndexCount, nInstanceCount, unStartIndex, 0, 0);
	}
}

QuadMesh::QuadMesh()
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nVertices = 6;

	std::vector<Vector3> vertices = {
		Vector3{ -1.f, 1.f,  0.f },
		Vector3{ 1.f, -1.f,  0.f },
		Vector3{ -1.f, -1.f, 0.f },
				 
		Vector3{ -1.f, 1.f, 0.f },
		Vector3{ 1.f, 1.f,  0.f },
		Vector3{ 1.f, -1.f, 0.f },
	};

	std::vector<Vector2> texCoords = {
		Vector2{ 0.f, 0.f },
		Vector2{ 1.f, 1.f },
		Vector2{ 0.f, 1.f },

		Vector2{ 0.f, 0.f },
		Vector2{ 1.f, 0.f },
		Vector2{ 1.f, 1.f },
	};

	m_Positions = RESOURCE->CreateVertexBuffer(vertices, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
	m_TexCoords = RESOURCE->CreateVertexBuffer(texCoords, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));

}

void QuadMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[3] = {
		m_Positions.VertexBufferView,
		m_TexCoords.VertexBufferView,
	};

	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);
	pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
}
