#include "pch.h"
#include "TerrainComponent.h"

void TerrainComponent::Initialize(const TERRAINCOMPONENTLOADINFO& componentLoadInfo, const Vector3& v3TerrainScale, const TerrainIndexRange& indexRange, const BoundingBox& xmBound)
{
	m_v2ComponentOriginXZ = componentLoadInfo.v2ComponentOriginXZ;
	m_xmi2NumQuadsXZ = componentLoadInfo.xmi2NumQuadsXZ;
	m_xmi4LayerIndices = componentLoadInfo.xmi4LayerIndices;
	m_WeightMapHandles = TEXTURE->LoadTexture(componentLoadInfo.strWeightMapName);
	m_v3TerrainScale = v3TerrainScale;
	m_IndexRange = indexRange;

	m_xmAABBComponentBounds = xmBound;
}

CB_TERRAIN_COMPONENT_DATA TerrainComponent::MakeCBData() const
{
	return CB_TERRAIN_COMPONENT_DATA{
		.v2ComponentOriginXZ = m_v2ComponentOriginXZ,
		.v2ComponentSizeXZ = Vector2{ (m_xmi2NumQuadsXZ.x) * m_v3TerrainScale.x, (m_xmi2NumQuadsXZ.y) * m_v3TerrainScale.z },
		.xmi4LayerIndex = m_xmi4LayerIndices,
		.xmi2NumQuadsXZ = m_xmi2NumQuadsXZ
	};
}

TerrainComponentData TerrainComponent::MakeTerrainComponentData(uint32 unWeightMapIndex) const
{
	return TerrainComponentData{
		.v2ComponentOriginXZ = m_v2ComponentOriginXZ,
		.v2ComponentSizeXZ = Vector2{ (m_xmi2NumQuadsXZ.x) * m_v3TerrainScale.x, (m_xmi2NumQuadsXZ.y) * m_v3TerrainScale.z },
		.xmi4LayerIndex = m_xmi4LayerIndices,
		.xmi2NumQuadsXZ = m_xmi2NumQuadsXZ,
		.nWeightMapIndex = unWeightMapIndex
	};
}
