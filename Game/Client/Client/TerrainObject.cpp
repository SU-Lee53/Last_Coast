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
	meshLoadInfo.eMeshType = MESH_TYPE::TERRAIN;

	float fHeightMapWidth = terrainInfo.v2HeightMapResolutionXZ.x;
	float fHeightMapHeight = terrainInfo.v2HeightMapResolutionXZ.y;
	uint32 unHeightMapWidth = static_cast<uint32>(fHeightMapWidth);
	uint32 unHeightMapHeight = static_cast<uint32>(fHeightMapHeight);

	size_t unVertices = static_cast<size_t>(fHeightMapWidth) * fHeightMapHeight;
	meshLoadInfo.v3Positions.resize(unVertices);
	meshLoadInfo.v3Normals.resize(unVertices);
	meshLoadInfo.v3Tangents.resize(unVertices);

	// Vertices
	/*
		fHeightMapWidth = 4;
		fHeightMapHeight = 4;

		(0, height)
		13---14---15---16
		|    |    |    |
		9----10---11---12
		|    |    |    |   ------> std::vector<Vector3> : [1][2][3][4] [5][6][7][8] [9][10][11][12] [13][14][15][16]
		5----6----7----8
		|    |    |    |
		1----2----3----4
		(0,0)          (width, 0)
	*/
	for (uint32 z = 0; z < unHeightMapHeight; ++z) {
		for (uint32 x = 0; x < unHeightMapWidth; ++x) {
			const float fLocalX = static_cast<float>(x) * terrainInfo.v3TerrainScale.x;
			const float fLocalZ = static_cast<float>(z) * terrainInfo.v3TerrainScale.z;

			Vector3 v3Position = { fLocalX, m_pHeightMapRawImage->GetHeightLocal(fLocalX, fLocalZ), fLocalZ };
			Vector3 v3Normal = m_pHeightMapRawImage->GetNormalLocal(fLocalX, fLocalZ);
			Vector3 v3Tangent = m_pHeightMapRawImage->GetTangentLocal(fLocalX, fLocalZ);

			size_t unIndex = (z * unHeightMapWidth) + x;
			meshLoadInfo.v3Positions[unIndex] = v3Position;
			meshLoadInfo.v3Normals[unIndex] = v3Normal;
			meshLoadInfo.v3Tangents[unIndex] = v3Tangent;
		}
	}

	// IndexRange
	auto fnVertexToIndex = [unHeightMapWidth](uint32 x, uint32 z) -> uint32 {
		return (z * unHeightMapWidth) + x;
	};

	uint32 unNumIndices{};
	for (const auto& componentInfo : terrainInfo.ComponentInfos) {
		unNumIndices += (componentInfo.xmi2NumQuadsXZ.x * componentInfo.xmi2NumQuadsXZ.y) * 6;
	}
	std::vector<uint32>& unIndices = meshLoadInfo.unIndices;
	unIndices.reserve(unNumIndices);

	m_pTerrainComponents.resize(terrainInfo.ComponentInfos.size());
	for (uint32 i = 0; i < terrainInfo.ComponentInfos.size(); ++i) {
		const auto& componentInfo = terrainInfo.ComponentInfos[i];
		TerrainIndexRange indexRange{};
		indexRange.unStartIndex = static_cast<uint32>(unIndices.size());

		// Component Origin -> HeightMap grid index
		const uint32 unBaseX = static_cast<uint32>(componentInfo.v2ComponentOriginXZ.x / terrainInfo.v3TerrainScale.x + 0.5f);
		const uint32 unBaseZ = static_cast<uint32>(componentInfo.v2ComponentOriginXZ.y / terrainInfo.v3TerrainScale.z + 0.5f);

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
		}
		indexRange.unIndexCount = static_cast<uint32>(unIndices.size()) - indexRange.unStartIndex;
		m_pTerrainComponents[i] = std::make_unique<TerrainComponent>();
		m_pTerrainComponents[i]->Initialize(componentInfo, terrainInfo.v3TerrainScale, indexRange);
	}

	// Material
	uint32 unLayers = terrainInfo.LayerInfos.size();
	std::vector<MATERIALLOADINFO> materialLoadInfos(unLayers);
	for (uint32 i = 0; i < unLayers; ++i) {
		materialLoadInfos[i].strAlbedoMapName = terrainInfo.LayerInfos[i].strAlbedoMapName;
		materialLoadInfos[i].strNormalMapName= terrainInfo.LayerInfos[i].strNormalMapName;
		materialLoadInfos[i].strTerrainLayerName= terrainInfo.LayerInfos[i].strLayerName;
		materialLoadInfos[i].unTerrainLayerIndex = terrainInfo.LayerInfos[i].unIndex;
		materialLoadInfos[i].fUVTiling = terrainInfo.LayerInfos[i].fTiling;
	}

	std::vector<MESHLOADINFO> meshLoadInfos = { meshLoadInfo };
	AddComponent<MeshRenderer>(meshLoadInfos, materialLoadInfos);
}

void TerrainObject::RenderImmediate(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	auto pMeshRenderer = GetComponent<MeshRenderer>();

	const auto& materials = pMeshRenderer->GetMaterials();
	float pfTiling[4] = { 0.f, 0.f, 0.f, 0.f };

	for (int i = 0; i < 4; ++i) {
		if (materials[i]) {
			const std::shared_ptr<TerrainMaterial> pTerrainMaterial = std::static_pointer_cast<TerrainMaterial>(materials[i]);
			pfTiling[i] = pTerrainMaterial->GetTiling();
		}
	}

	Vector4 v4Tiling = Vector4(pfTiling);
	ConstantBuffer& layerCBuffer = RESOURCE->AllocCBuffer<CB_TERRAIN_LAYER_DATA>();
	layerCBuffer.WriteData(&v4Tiling);

	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, layerCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	for (int i = 0; i < 4; ++i) {
		if (materials[i]) {
			const std::shared_ptr<TerrainMaterial> pTerrainMaterial = std::static_pointer_cast<TerrainMaterial>(materials[i]);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = descHandle.cpuHandle;

			pd3dDevice->CopyDescriptorsSimple(1, cpuHandle, pTerrainMaterial->GetTexture(0)->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpuHandle.Offset(4, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			pd3dDevice->CopyDescriptorsSimple(1, cpuHandle, pTerrainMaterial->GetTexture(1)->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}
		else {
			descHandle.cpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}
	}
	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::SCENE_TERRAIN_DATA), descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(8, D3DCore::g_nCBVSRVDescriptorIncrementSize);


	for (uint32 i = 0; i < m_pTerrainComponents.size(); ++i) {
		ConstantBuffer& cbuffer = RESOURCE->AllocCBuffer<CB_TERRAIN_COMPONENT_DATA>();
		CB_TERRAIN_COMPONENT_DATA terrainData = m_pTerrainComponents[i]->MakeCBData();
		cbuffer.WriteData(&terrainData);
		pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTerrainComponents[i]->GetWeightMap()->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::SCENE_TERRAIN_COMPONENT_DATA), descHandle.gpuHandle);
		descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		const auto& indexRange = m_pTerrainComponents[i]->GetIndexRange();
		int nInstanceBase = -1;
		Matrix mtxWorld = GetTransform()->GetWorldMatrix();
		pMeshRenderer->Render(pd3dDevice, pd3dCommandList, descHandle, indexRange.unStartIndex, indexRange.unIndexCount, 1, nInstanceBase, mtxWorld);
	}
}
