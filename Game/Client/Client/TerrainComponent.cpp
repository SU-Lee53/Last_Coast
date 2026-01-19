#include "pch.h"
#include "TerrainComponent.h"

void TerrainComponent::Initialize(const TERRAINCOMPONENTLOADINFO& componentLoadInfo, const TerrainIndexRange& indexRange)
{
	m_v2ComponentOriginXZ = componentLoadInfo.v2ComponentOriginXZ;
	m_xmi2NumQuadsXZ = componentLoadInfo.xmi2NumQuadsXZ;
	m_xmi4LayerIndices = componentLoadInfo.xmi4LayerIndices;
	m_pWeightMap = TEXTURE->LoadTexture(componentLoadInfo.strWeightMapName);
	m_IndexRange = indexRange;
}
