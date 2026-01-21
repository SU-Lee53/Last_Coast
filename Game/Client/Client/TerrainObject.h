#pragma once
#include "GameObject.h"
#include "TerrainComponent.h"
#include "HeightMapRawImage.h";

struct TERRAINLAYERLOADINFO {
	uint32 unIndex;
	float fTiling;
	std::string strLayerName;
	std::string strAlbedoMapName;
	std::string strNormalMapName;
};

struct TERRAINLOADINFO {
	Vector3 v3TerrainScale;
	Vector2 v2HeightMapResolutionXZ;
	float fHeightScale;
	std::string strHeightMapName;

	std::vector<TERRAINLAYERLOADINFO> LayerInfos;
	std::vector<TERRAINCOMPONENTLOADINFO> ComponentInfos;
};

class TerrainObject : public GameObject {
public:
	virtual void RenderImmediate(ComPtr<ID3D12Device> pd3dDevice,
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		DescriptorHandle& descHandle) override;

public:
	HRESULT LoadFromFiles(const std::string& strFilename);

private:
	void BuildTerrainMesh(const TERRAINLOADINFO& terrainInfo);

private:
	std::unique_ptr<HeightMapRawImage> m_pHeightMapRawImage;
	std::vector<std::unique_ptr<TerrainComponent>> m_pTerrainComponents;
	Vector3 m_v3TerrainScale;

	const static std::string g_strTerrainPath;
};

