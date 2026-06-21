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

void IMesh::RenderPosition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
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

void IMesh::ShowControlImGui()
{
	ImGui::Text("numPositions : %d", m_Positions.nVertices);
	ImGui::Text("numIndices : %d", m_IndexBuffer.nIndices);
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

void StaticMesh::ShowControlImGui() 
{
	IMesh::ShowControlImGui();
	ImGui::Text("numNormals : %d", m_Normals.nVertices);
	ImGui::Text("numTangents : %d", m_Tangents.nVertices);
	ImGui::Text("numTexCoords : %d", m_TexCoords.nVertices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinnedMesh

SkinnedMesh::SkinnedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology)
	: StaticMesh(meshLoadInfo, d3dTopology)
{
	m_BlendIndices = RESOURCE->CreateVertexBuffer(meshLoadInfo.xmun4BlendIndices, std::to_underlying(MESH_ELEMENT_TYPE::TANGENT));
	m_BlendWeights = RESOURCE->CreateVertexBuffer(meshLoadInfo.v4BlendWeights, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));
}

void SkinnedMesh::RenderPosition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[6] = {
		m_Positions.VertexBufferView,
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

void SkinnedMesh::ShowControlImGui()
{
	StaticMesh::ShowControlImGui();
	ImGui::Text("numTangents : %d", m_BlendIndices.nVertices);
	ImGui::Text("numTexCoords : %d", m_BlendWeights.nVertices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainMesh

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

void TerrainMesh::ShowControlImGui()
{
	IMesh::ShowControlImGui();
	ImGui::Text("numNormals : %d", m_Normals.nVertices);
	ImGui::Text("numTangents : %d", m_Tangents.nVertices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GridMesh

GridMesh::GridMesh(uint32 unNumQuadsX, uint32 unNumQuadsZ, float fSizeX, float fSizeZ, float fUVTiling)
{
	MESHLOADINFO meshLoadInfo = CreateLoadInfo(unNumQuadsX, unNumQuadsZ, fSizeX, fSizeZ, fUVTiling, MESH_TYPE::STATIC);

	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nVertices = meshLoadInfo.v3Positions.size();
	m_xmOBB.Center = meshLoadInfo.v3AABBCenter;
	m_xmOBB.Extents = meshLoadInfo.v3AABBExtents;

	m_Positions = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Positions, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
	m_Normals = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Normals, std::to_underlying(MESH_ELEMENT_TYPE::NORMAL));
	m_Tangents = RESOURCE->CreateVertexBuffer(meshLoadInfo.v3Tangents, std::to_underlying(MESH_ELEMENT_TYPE::TANGENT));
	m_TexCoords = RESOURCE->CreateVertexBuffer(meshLoadInfo.v2TexCoord0, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));
	m_IndexBuffer = RESOURCE->CreateIndexBuffer(meshLoadInfo.unIndices);
}

MESHLOADINFO GridMesh::CreateLoadInfo(uint32 unNumQuadsX, uint32 unNumQuadsZ, float fSizeX, float fSizeZ, float fUVTiling, MESH_TYPE eMeshType)
{
	unNumQuadsX = std::max<uint32>(1, unNumQuadsX);
	unNumQuadsZ = std::max<uint32>(1, unNumQuadsZ);
	fSizeX = std::max(0.001f, fSizeX);
	fSizeZ = std::max(0.001f, fSizeZ);

	const uint32 unNumVerticesX = unNumQuadsX + 1;
	const uint32 unNumVerticesZ = unNumQuadsZ + 1;
	const float fIntervalX = fSizeX / static_cast<float>(unNumQuadsX);
	const float fIntervalZ = fSizeZ / static_cast<float>(unNumQuadsZ);

	MESHLOADINFO meshLoadInfo{};
	meshLoadInfo.strMeshName = "GridMesh";
	meshLoadInfo.eMeshType = eMeshType;
	meshLoadInfo.v3AABBCenter = Vector3(fSizeX * 0.5f, 0.0f, fSizeZ * 0.5f);
	meshLoadInfo.v3AABBExtents = Vector3(fSizeX * 0.5f, 0.01f, fSizeZ * 0.5f);

	const uint32 unNumVertices = unNumVerticesX * unNumVerticesZ;
	meshLoadInfo.v3Positions.reserve(unNumVertices);
	meshLoadInfo.v3Normals.reserve(unNumVertices);
	meshLoadInfo.v3Tangents.reserve(unNumVertices);
	meshLoadInfo.v2TexCoord0.reserve(unNumVertices);
	meshLoadInfo.unIndices.reserve(static_cast<size_t>(unNumQuadsX) * unNumQuadsZ * 6);

	for (uint32 z = 0; z < unNumVerticesZ; ++z) {
		const float fPosZ = fIntervalZ * static_cast<float>(z);
		const float fUV = (static_cast<float>(z) / static_cast<float>(unNumQuadsZ)) * fUVTiling;

		for (uint32 x = 0; x < unNumVerticesX; ++x) {
			const float fPosX = fIntervalX * static_cast<float>(x);
			const float fU = (static_cast<float>(x) / static_cast<float>(unNumQuadsX)) * fUVTiling;

			meshLoadInfo.v3Positions.emplace_back(fPosX, 0.0f, fPosZ);
			meshLoadInfo.v3Normals.emplace_back(0.0f, 1.0f, 0.0f);
			meshLoadInfo.v3Tangents.emplace_back(1.0f, 0.0f, 0.0f);
			meshLoadInfo.v2TexCoord0.emplace_back(fU, fUV);
		}
	}

	auto fnVertexToIndex = [unNumVerticesX](uint32 x, uint32 z) -> uint32 {
		return z * unNumVerticesX + x;
	};

	for (uint32 z = 0; z < unNumQuadsZ; ++z) {
		for (uint32 x = 0; x < unNumQuadsX; ++x) {
			const uint32 v0 = fnVertexToIndex(x, z);
			const uint32 v1 = fnVertexToIndex(x + 1, z);
			const uint32 v2 = fnVertexToIndex(x, z + 1);
			const uint32 v3 = fnVertexToIndex(x + 1, z + 1);

			meshLoadInfo.unIndices.push_back(v0);
			meshLoadInfo.unIndices.push_back(v2);
			meshLoadInfo.unIndices.push_back(v1);

			meshLoadInfo.unIndices.push_back(v1);
			meshLoadInfo.unIndices.push_back(v2);
			meshLoadInfo.unIndices.push_back(v3);
		}
	}

	return meshLoadInfo;
}

void GridMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[4] = {
		m_Positions.VertexBufferView,
		m_Normals.VertexBufferView,
		m_Tangents.VertexBufferView,
		m_TexCoords.VertexBufferView
	};
	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);

	const uint32 unIndices = (nIndexCount == -1) ? m_IndexBuffer.nIndices : nIndexCount;
	pd3dCommandList->IASetIndexBuffer(&m_IndexBuffer.IndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(unIndices, nInstanceCount, unStartIndex, 0, 0);
}

void GridMesh::ShowControlImGui()
{
	IMesh::ShowControlImGui();
	ImGui::Text("numNormals : %d", m_Normals.nVertices);
	ImGui::Text("numTangents : %d", m_Tangents.nVertices);
	ImGui::Text("numTexCoords : %d", m_TexCoords.nVertices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QuadMesh

QuadMesh::QuadMesh(float fMin, float fMax)
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nVertices = 6;

	std::vector<Vector3> vertices = {
		Vector3{ fMin, fMax,  0.f },
		Vector3{ fMax, fMin,  0.f },
		Vector3{ fMin, fMin, 0.f },
				 
		Vector3{ fMin, fMax, 0.f },
		Vector3{ fMax, fMax,  0.f },
		Vector3{ fMax, fMin, 0.f },
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CubeMesh

CubeMesh::CubeMesh(Vector3 v3Extents)
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nVertices = 36;

	float fMinX = -v3Extents.x, fMaxX = v3Extents.x;
	float fMinY = -v3Extents.y, fMaxY = v3Extents.y;
	float fMinZ = -v3Extents.z, fMaxZ = v3Extents.z;

	std::vector<Vector3> vertices = {
		// +X
		Vector3{ fMaxX, fMaxY, fMinZ }, Vector3{ fMaxX, fMaxY, fMaxZ }, Vector3{ fMaxX, fMinY, fMinZ },
		Vector3{ fMaxX, fMaxY, fMaxZ }, Vector3{ fMaxX, fMinY, fMaxZ }, Vector3{ fMaxX, fMinY, fMinZ },

		// -X
		Vector3{ fMinX, fMaxY, fMaxZ }, Vector3{ fMinX, fMaxY, fMinZ }, Vector3{ fMinX, fMinY, fMaxZ },
		Vector3{ fMinX, fMaxY, fMinZ }, Vector3{ fMinX, fMinY, fMinZ }, Vector3{ fMinX, fMinY, fMaxZ },

		// +Y
		Vector3{ fMinX, fMaxY, fMaxZ }, Vector3{ fMaxX, fMaxY, fMaxZ }, Vector3{ fMinX, fMaxY, fMinZ },
		Vector3{ fMaxX, fMaxY, fMaxZ }, Vector3{ fMaxX, fMaxY, fMinZ }, Vector3{ fMinX, fMaxY, fMinZ },

		// -Y
		Vector3{ fMinX, fMinY, fMinZ }, Vector3{ fMaxX, fMinY, fMinZ }, Vector3{ fMinX, fMinY, fMaxZ },
		Vector3{ fMaxX, fMinY, fMinZ }, Vector3{ fMaxX, fMinY, fMaxZ }, Vector3{ fMinX, fMinY, fMaxZ },

		// +Z
		Vector3{ fMaxX, fMaxY, fMaxZ }, Vector3{ fMinX, fMaxY, fMaxZ }, Vector3{ fMaxX, fMinY, fMaxZ },
		Vector3{ fMinX, fMaxY, fMaxZ }, Vector3{ fMinX, fMinY, fMaxZ }, Vector3{ fMaxX, fMinY, fMaxZ },

		// -Z
		Vector3{ fMinX, fMaxY, fMinZ }, Vector3{ fMaxX, fMaxY, fMinZ }, Vector3{ fMinX, fMinY, fMinZ },
		Vector3{ fMaxX, fMaxY, fMinZ }, Vector3{ fMaxX, fMinY, fMinZ }, Vector3{ fMinX, fMinY, fMinZ },
	};
	std::array<Vector2, 6> faceUV = {
		Vector2{0.f, 0.f}, Vector2{1.f, 0.f}, Vector2{0.f, 1.f},
		Vector2{1.f, 0.f}, Vector2{1.f, 1.f}, Vector2{0.f, 1.f}
	};

	std::vector<Vector2> texCoords;
	texCoords.reserve(36);
	for (int i = 0; i < 6; ++i) {
		texCoords.insert(texCoords.cbegin(), faceUV.begin(), faceUV.end());
	}

	m_Positions = RESOURCE->CreateVertexBuffer(vertices, std::to_underlying(MESH_ELEMENT_TYPE::POSITION));
	m_TexCoords = RESOURCE->CreateVertexBuffer(texCoords, std::to_underlying(MESH_ELEMENT_TYPE::TEXCOORD0));

}

void CubeMesh::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 nInstanceCount, uint32 unStartIndex, int32 nIndexCount) const
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[3] = {
		m_Positions.VertexBufferView,
		m_TexCoords.VertexBufferView,
	};

	pd3dCommandList->IASetVertexBuffers(0, _countof(vertexBufferViews), vertexBufferViews);
	pd3dCommandList->DrawInstanced(m_Positions.nVertices, nInstanceCount, 0, 0);
}
