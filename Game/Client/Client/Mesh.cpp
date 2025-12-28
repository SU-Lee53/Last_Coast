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

	m_PositionBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Positions, MESH_ELEMENT_TYPE_POSITION);

	m_xmOBB.Center = meshLoadInfo.xmf3AABBCenter;
	m_xmOBB.Extents= meshLoadInfo.xmf3AABBExtents;

	// IB
	m_IndexBuffers.reserve(meshLoadInfo.SubMeshes.size());
	for (const auto& indices : meshLoadInfo.SubMeshes) {
		m_IndexBuffers.push_back(RESOURCE->CreateIndexBuffer(indices));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffusedMesh 

DiffusedMesh::DiffusedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: Mesh(meshLoadInfo, d3dTopology)
{
	m_ColorBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v4Colors, MESH_ELEMENT_TYPE_COLOR);
}

void DiffusedMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2] = { m_PositionBuffer.VertexBufferView, m_ColorBuffer.VertexBufferView };
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffers.size() != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffers[nSubSet].IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffers[nSubSet].nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_PositionBuffer.nVertices, nInstanceCount, 0, 0);
	}

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
	pd3dCommandList->IASetVertexBuffers(0, 1, &m_PositionBuffer.VertexBufferView);

	if (m_IndexBuffers.size() != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffers[nSubSet].IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffers[nSubSet].nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_PositionBuffer.nVertices, nInstanceCount, 0, 0);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedMesh

TexturedMesh::TexturedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: Mesh(meshLoadInfo, d3dTopology)
{
	m_TexCoordBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v2TexCoord0, MESH_ELEMENT_TYPE_TEXCOORD0);
}

void TexturedMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[2] = { m_PositionBuffer.VertexBufferView, m_TexCoordBuffer.VertexBufferView };
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffers.size() != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffers[nSubSet].IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffers[nSubSet].nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_PositionBuffer.nVertices, nInstanceCount, 0, 0);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalMesh 

TexturedNormalMesh::TexturedNormalMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: TexturedMesh(meshLoadInfo, d3dTopology)
{
	m_NormalBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Normals, MESH_ELEMENT_TYPE_COLOR);
	m_TexCoordBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v2TexCoord0, MESH_ELEMENT_TYPE_TEXCOORD0);
}

void TexturedNormalMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[3] = { m_PositionBuffer.VertexBufferView, m_NormalBuffer.VertexBufferView, m_TexCoordBuffer.VertexBufferView };
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffers.size() != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffers[nSubSet].IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffers[nSubSet].nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_PositionBuffer.nVertices, nInstanceCount, 0, 0);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalTangentMesh

TexturedNormalTangentMesh::TexturedNormalTangentMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: TexturedNormalMesh(meshLoadInfo, d3dTopology)
{
	m_TangentBuffer = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, MESH_ELEMENT_TYPE_COLOR);
}

void TexturedNormalTangentMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[4] = { m_PositionBuffer.VertexBufferView, m_NormalBuffer.VertexBufferView, m_TangentBuffer.VertexBufferView, m_TexCoordBuffer.VertexBufferView };
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	if (m_IndexBuffers.size() != 0) {
		pd3dCommandList->IASetIndexBuffer(&m_IndexBuffers[nSubSet].IndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_IndexBuffers[nSubSet].nIndices, nInstanceCount, 0, 0, 0);
	}
	else {
		pd3dCommandList->DrawInstanced(m_PositionBuffer.nVertices, nInstanceCount, 0, 0);
	}

}
