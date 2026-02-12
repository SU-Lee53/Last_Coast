#pragma once
#include "ShaderResource.h"

/*
	- Mesh 개편
		- 서로 다른 Input Element는 별개의 정점버퍼를 갖도록 한다
			- 이유 : 한번에 모든 Input Element를 하나의 VB로 묶으면 
			         Prepass 등에서 원하는 요소만 바인딩할 수 없음

		- 기존 방식도 일단 유지
*/

enum class MESH_TYPE {
	STATIC = 0,
	SKINNED,
	TERRAIN,


	COUNT,

	UNDEFINED
};

struct MESHLOADINFO {
	std::string				strMeshName;

	Vector3					v3AABBCenter = Vector3(0.0f, 0.0f, 0.0f);
	Vector3					v3AABBExtents = Vector3(1.0f, 1.0, 1.0f);

	std::vector<Vector3>	v3Positions;
	std::vector<Vector4>	v4Colors;
	std::vector<Vector3>	v3Normals;
	std::vector<Vector3>	v3Tangents;

	std::vector<Vector2>	v2TexCoord0;
	std::vector<Vector2>	v2TexCoord1;
	std::vector<Vector2>	v2TexCoord2;
	std::vector<Vector2>	v2TexCoord3;

	std::vector<XMUINT4>	xmun4BlendIndices;
	std::vector<Vector4>	v4BlendWeights;

	std::vector<uint32>		unIndices;

	MESH_TYPE				eMeshType;

	bool bIsSkinned = false;
};

interface IMesh abstract {
public:
	using ID = uint64;

public:
	IMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	virtual ~IMesh() {}
	
	const BoundingOrientedBox& GetBoundingBox() const { return m_xmOBB; }

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		uint32 unStartIndex, 
		uint32 unIndexCount = std::numeric_limits<uint32>::max(),
		uint32 nInstanceCount = 1) const = 0;

protected:
	VertexBuffer					m_Positions;
	IndexBuffer						m_IndexBuffer;

protected:
	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology;
	uint32							m_nSlot = 0;
	uint32							m_nVertices = 0;
	uint32							m_nOffset = 0;

protected:
	// Bounding Volume
	BoundingOrientedBox m_xmOBB;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenMesh

class FullScreenMesh : public IMesh {
public:
	FullScreenMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		uint32 unStartIndex,
		uint32 unIndexCount = std::numeric_limits<uint32>::max(),
		uint32 nInstanceCount = 1) const = 0;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StaticMesh

class StaticMesh : public IMesh {
public:
	StaticMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		uint32 unStartIndex,
		uint32 unIndexCount = std::numeric_limits<uint32>::max(),
		uint32 nInstanceCount = 1) const override;

protected:
	VertexBuffer m_Normals;
	VertexBuffer m_Tangents;
	VertexBuffer m_TexCoords;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinnedMesh

class SkinnedMesh : public StaticMesh {
public:
	SkinnedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		uint32 unStartIndex,
		uint32 unIndexCount = std::numeric_limits<uint32>::max(),
		uint32 nInstanceCount = 1) const override;

protected:
	VertexBuffer m_BlendIndices;
	VertexBuffer m_BlendWeights;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TerrainMesh

class TerrainMesh : public IMesh {
public:
	TerrainMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		uint32 unStartIndex,
		uint32 unIndexCount = std::numeric_limits<uint32>::max(),
		uint32 nInstanceCount = 1) const override;

protected:
	VertexBuffer m_Normals;
	VertexBuffer m_Tangents;
};

