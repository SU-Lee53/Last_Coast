#pragma once

struct TERRAINCOMPONENTLOADINFO {
	uint32 unComponentIndex;
	Vector2 v2ComponentOriginXZ;
	XMINT2 xmi2NumQuadsXZ;
	XMINT4 xmi4LayerIndices;
	std::string strWeightMapName;

	const static DXGI_FORMAT g_dxgiWeightMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};

struct TerrainIndexRange {
	uint32 unStartIndex = 0;
	uint32 unIndexCount = 0;
};

struct CB_TERRAIN_LAYER_DATA {
	Vector4 v4LayerTiling;
	int32 nLayers;
};

struct CB_TERRAIN_COMPONENT_DATA {
	Vector2	v2ComponentOriginXZ;
	Vector2	v2ComponentSizeXZ;	// 까지 c0
	XMINT4	xmi4LayerIndex;		// c1
	XMINT2	xmi2NumQuadsXZ;		// c2.xy
};

class TerrainComponent {
public:
	void Initialize(const TERRAINCOMPONENTLOADINFO& componentLoadInfo, const Vector3& v3TerrainScale, const TerrainIndexRange& indexRange);
	CB_TERRAIN_COMPONENT_DATA MakeCBData() const;
	const TerrainIndexRange& GetIndexRange() const { return m_IndexRange; };
	std::shared_ptr<Texture> GetWeightMap() const { return m_pWeightMap; }

	Vector2 GetComponentSize() const { return Vector2{ (m_xmi2NumQuadsXZ.x) * m_v3TerrainScale.x, (m_xmi2NumQuadsXZ.y) * m_v3TerrainScale.z }; }

private:
	Vector2 m_v2ComponentOriginXZ = Vector2{ 0.f, 0.f };
	XMINT2 m_xmi2NumQuadsXZ = XMINT2{ 0,0 };
	XMINT4 m_xmi4LayerIndices = XMINT4{ 0,0,0,0 };
	Vector3 m_v3TerrainScale;
	std::shared_ptr<Texture> m_pWeightMap;
	
	TerrainIndexRange m_IndexRange{};

};

