#include "pch.h"
#include "Mesh.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mesh 

Mesh::Mesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
{
	m_d3dPrimitiveTopology = d3dTopology;
	m_nSlot = 0;
	m_nVertices = meshLoadInfo.v3Positions.size();
	m_nOffset = 0;
	m_nType = meshLoadInfo.nType;

	m_Positions = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Positions, MESH_ELEMENT_TYPE_POSITION);

	m_xmOBB.Center = meshLoadInfo.v3AABBCenter;
	m_xmOBB.Extents= meshLoadInfo.v3AABBExtents;

	// IB
	m_IndexBuffer = RESOURCE->CreateIndexBuffer(meshLoadInfo.uiIndices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenMesh

FullScreenMesh::FullScreenMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: Mesh(meshLoadInfo, d3dTopology)
{
}

void FullScreenMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_Positions.VertexBufferView);

	if (m_IndexBuffer.nIndices != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffer.nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardMesh

StandardMesh::StandardMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: Mesh(meshLoadInfo, d3dTopology)
{
	m_Normals = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, MESH_ELEMENT_TYPE_NORMAL);
	m_Tangents = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, MESH_ELEMENT_TYPE_TANGENT);
	m_TexCoords = RESOURCE->CreateVertexBuffer(meshLoadInfo.v2TexCoord0, MESH_ELEMENT_TYPE_TEXCOORD0);
}

void StandardMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
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
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffer.nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinnedMesh

SkinnedMesh::SkinnedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: StandardMesh(meshLoadInfo, d3dTopology)
{
	m_BlendIndices = RESOURCE->CreateVertexBuffer(meshLoadInfo.xmui4BlendIndices, MESH_ELEMENT_TYPE_TANGENT);
	m_BlendWeights = RESOURCE->CreateVertexBuffer(meshLoadInfo.v4BlendWeights, MESH_ELEMENT_TYPE_TEXCOORD0);
}

void SkinnedMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
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
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffer.nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
	}

}
