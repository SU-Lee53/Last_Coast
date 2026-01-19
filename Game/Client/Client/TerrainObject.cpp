#include "pch.h"
#include "TerrainObject.h"

const std::string TerrainObject::g_strTerrainPath = "../Models/Terrain";

HRESULT TerrainObject::LoadFromFiles(const std::string& strFilename)
{
	HRESULT hr{};
	TERRAINLOADINFO terrainInfo{};
	// 1. Load from Json file


	// 2. Heightmap
	m_pHeightMapRawImage = std::make_unique<HeightMapRawImage>();
	hr = m_pHeightMapRawImage->LoadFromFile(
		terrainInfo.strHeightMapName,
		terrainInfo.v2HeightMapResolutionXZ.x,
		terrainInfo.v2HeightMapResolutionXZ.y,
		0.f,
		0.f,
		terrainInfo.v3TerrainScale.x,
		terrainInfo.v3TerrainScale.z,
		terrainInfo.fHeightScale
	);

	if (FAILED(hr)) {
		__debugbreak();
		return E_FAIL;
	}

	// TerrainMesh + Component
	BuildTerrainMesh(terrainInfo);

	return S_OK;
}

void TerrainObject::BuildTerrainMesh(const TERRAINLOADINFO& terrainInfo)
{
	MESHLOADINFO meshLoadInfo{};
	float fHeightMapWidth = terrainInfo.v2HeightMapResolutionXZ.x;
	float fHeightMapHeight = terrainInfo.v2HeightMapResolutionXZ.y;

	size_t unVertices = static_cast<size_t>(fHeightMapWidth) * fHeightMapHeight;
	meshLoadInfo.v3Positions.resize(unVertices);
	meshLoadInfo.v3Normals.resize(unVertices);
	meshLoadInfo.v3Tangents.resize(unVertices);

	// Vertices
	for (uint32 z = 0; z < fHeightMapHeight; ++z) {
		for (uint32 x = 0; x < fHeightMapWidth; ++x) {
			const float fLocalX = static_cast<float>(x) * terrainInfo.v3TerrainScale.x;
			const float fLocalZ = static_cast<float>(z) * terrainInfo.v3TerrainScale.z;

			Vector3 v3Position = { fLocalX, m_pHeightMapRawImage->GetHeightLocal(fLocalX, fLocalZ), fLocalZ };
			Vector3 v3Normal = m_pHeightMapRawImage->GetNormalLocal(fLocalX, fLocalZ);
			Vector3 v3Tangent = m_pHeightMapRawImage->GetTangentLocal(fLocalX, fLocalZ);

			size_t unIndex = static_cast<size_t>(z) * fHeightMapWidth + x;
			meshLoadInfo.v3Positions[unIndex] = v3Position;
			meshLoadInfo.v3Normals[unIndex] = v3Normal;
			meshLoadInfo.v3Tangents[unIndex] = v3Tangent;
		}
	}

	// IndexRange
	auto fnVertexToIndex = [fHeightMapWidth](uint32 x, uint32 z) -> uint32 {
		return (z * fHeightMapWidth) + x;
		};

	uint32 unNumIndices{};
	for (const auto& componentInfo : terrainInfo.ComponentInfos) {
		unNumIndices += (componentInfo.xmi2NumQuadsXZ.x * componentInfo.xmi2NumQuadsXZ.y) * 6;
	}
	std::vector<uint32>& unIndices = meshLoadInfo.unIndices;
	unIndices.reserve(unNumIndices);

	m_pTerrainComponents.reserve(terrainInfo.ComponentInfos.size());

	for (const auto& componentInfo : terrainInfo.ComponentInfos) {
		TerrainIndexRange indexRange{};
		indexRange.unStartIndex = static_cast<uint32>(unIndices.size());

		// Component Origin -> HeightMap grid index
		const uint32 unBaseX = static_cast<uint32>(std::round(componentInfo.v2ComponentOriginXZ.x / terrainInfo.v3TerrainScale.x));
		const uint32 unBaseZ = static_cast<uint32>(std::round(componentInfo.v2ComponentOriginXZ.y / terrainInfo.v3TerrainScale.z));

		for (uint32 z = 0; z < componentInfo.xmi2NumQuadsXZ.y; ++z) {
			for (uint32 x = 0; x < componentInfo.xmi2NumQuadsXZ.x; ++x) {
				const uint32 x0 = unBaseX + x;
				const uint32 z0 = unBaseZ + z;

				const uint32 v0 = fnVertexToIndex(x0, z0);
				const uint32 v1 = fnVertexToIndex(x0 + 1, z0);
				const uint32 v2 = fnVertexToIndex(x0, z0 + 1);
				const uint32 v3 = fnVertexToIndex(x0 + 1, z0 + 1);


				/*
					v2      v3
					+-------+
					|       |   -> (v0, v2, v1), (v1, v2, v3)
					|       |
					+-------+
					v0      v1
				*/

				unIndices.insert(unIndices.end(), { v0, v2, v1 });
				unIndices.insert(unIndices.end(), { v1, v2, v3 });
			}

			indexRange.unIndexCount = static_cast<uint32>(unIndices.size()) - indexRange.unStartIndex;
			m_pTerrainComponents.emplace_back(componentInfo, indexRange);
		}
	}
	std::shared_ptr<TerrainMesh> pTerrainMesh = std::make_shared<TerrainMesh>(meshLoadInfo);

	// Material
	uint32 unLayers = terrainInfo.LayerInfos.size();
	std::vector<std::shared_ptr<TerrainMaterial>> pTerrainMaterials;
	pTerrainMaterials.reserve(unLayers);
	for (uint32 i = 0; i < unLayers; ++i) {
		MATERIALLOADINFO materialLoadInfo{};
		const TERRAINLAYERLOADINFO& layerInfo = terrainInfo.LayerInfos[i];
		materialLoadInfo.strAlbedoMapName = layerInfo.strAlbedoMapName;
		materialLoadInfo.strNormalMapName= layerInfo.strNormalMapName;
		pTerrainMaterials.emplace_back(materialLoadInfo, layerInfo.strLayerName, layerInfo.unIndex, layerInfo.fTiling);
	}

	std::vector<std::shared_ptr<TerrainMesh>> pTerrainMeshes = { pTerrainMesh };
	m_pMeshRenderer = std::make_shared<MeshRenderer<TerrainMesh>>(pTerrainMeshes, pTerrainMaterials);

}
