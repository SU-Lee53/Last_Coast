#pragma once
#include "ShaderResource.h"

/*
	- Mesh 개편
		- 서로 다른 Input Element는 별개의 정점버퍼를 갖도록 한다
			- 이유 : 한번에 모든 Input Element를 하나의 VB로 묶으면 
			         Prepass 등에서 원하는 요소만 바인딩할 수 없음

		- 기존 방식도 일단 유지
*/

struct MESHLOADINFO {
	std::string				strMeshName;

	UINT					nType;			// MESH_ELEMENT_TYPE

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

	std::vector<XMINT4>		xmi4BlendIndices;
	std::vector<Vector4>	v4BlendWeights;

	std::vector<UINT>		uiIndices;

	bool bIsSkinned = false;
};

class Mesh {
public:
	Mesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual ~Mesh() {}

	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount = 1) const = 0;

protected:
	VertexBuffer					m_Positions;
	IndexBuffer						m_IndexBuffer;	

protected:
	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology;
	UINT							m_nSlot = 0;
	UINT							m_nVertices = 0;
	UINT							m_nOffset = 0;

	UINT							m_nType = 0;

protected:
	// Bounding Volume
	BoundingOrientedBox m_xmOBB;

public:
	static std::shared_ptr<Mesh> LoadMeshFromFile(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const nlohmann::json& inJson);
	

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FullScreenMesh

class FullScreenMesh : public Mesh {
public:
	FullScreenMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount = 1) const override;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StandardMesh

class StandardMesh : public Mesh {
public:
	StandardMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount = 1) const override;

protected:
	VertexBuffer m_Normals;
	VertexBuffer m_Tangents;
	VertexBuffer m_TexCoords;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinnedMesh
// TODO : 구현
// 구현 완료 후 위에 잡다한거 날릴것

class SkinnedMesh : public StandardMesh {
public:
	SkinnedMesh(const MESHLOADINFO& meshLoadInfo, D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nSubSet, UINT nInstanceCount = 1) const override;

protected:
	VertexBuffer m_BlendIndices;
	VertexBuffer m_BlendWeights;
};
