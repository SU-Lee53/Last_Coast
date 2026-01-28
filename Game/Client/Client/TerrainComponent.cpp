#include "pch.h"
#include "TerrainComponent.h"

void TerrainComponent::Initialize(const TERRAINCOMPONENTLOADINFO& componentLoadInfo, const Vector3& v3TerrainScale, const TerrainIndexRange& indexRange)
{
	m_v2ComponentOriginXZ = componentLoadInfo.v2ComponentOriginXZ;
	m_xmi2NumQuadsXZ = componentLoadInfo.xmi2NumQuadsXZ;
	m_xmi4LayerIndices = componentLoadInfo.xmi4LayerIndices;
	m_pWeightMap = TEXTURE->LoadTexture(componentLoadInfo.strWeightMapName);
	//m_pWeightMap = TEXTURE->LoadTextureFromRaw(componentLoadInfo.strWeightMapName, m_xmi2NumQuadsXZ.x + 1, , m_xmi2NumQuadsXZ.y + 1);
	m_v3TerrainScale = v3TerrainScale;
	m_IndexRange = indexRange;
}

CB_TERRAIN_COMPONENT_DATA TerrainComponent::MakeCBData() const
{
	CB_TERRAIN_COMPONENT_DATA data{};
	data.v2ComponentOriginXZ = m_v2ComponentOriginXZ;
	data.v2ComponentSizeXZ = Vector2{ (m_xmi2NumQuadsXZ.x) * m_v3TerrainScale.x, (m_xmi2NumQuadsXZ.y) * m_v3TerrainScale.z };
	data.xmi4LayerIndex = m_xmi4LayerIndices;
	data.xmi2NumQuadsXZ = m_xmi2NumQuadsXZ;

	return data;
}
