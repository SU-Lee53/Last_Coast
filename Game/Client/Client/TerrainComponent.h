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

struct CB_TERRAIN_COMPONENT_DATA {

};

class TerrainComponent {
public:
	void Initialize(const TERRAINCOMPONENTLOADINFO& componentLoadInfo, const TerrainIndexRange& indexRange);

private:
	Vector2 m_v2ComponentOriginXZ = Vector2{ 0.f, 0.f };
	XMINT2 m_xmi2NumQuadsXZ = XMINT2{ 0,0 };
	XMINT4 m_xmi4LayerIndices = XMINT4{ 0,0,0,0 };
	std::shared_ptr<Texture> m_pWeightMap;
	
	TerrainIndexRange m_IndexRange{};

};

