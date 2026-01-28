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

class TerrainObject : public IGameObject {
public:
	virtual void RenderImmediate(ComPtr<ID3D12Device> pd3dDevice,
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		DescriptorHandle& descHandle) override;


	const auto& GetTerrainComponents() const { return m_pTerrainComponents; }

public:
	HRESULT LoadFromFiles(const std::string& strFilename);

private:
	void BuildTerrainMesh(const TERRAINLOADINFO& terrainInfo);
	void ReadTerrainData(const nlohmann::json& j, OUT TERRAINLOADINFO& outTerrainInfo);
	TERRAINCOMPONENTLOADINFO ReadTerrainComponentData(const nlohmann::json& j);
	TERRAINLAYERLOADINFO ReadTerrainLayerData(const nlohmann::json& j);

private:
	std::unique_ptr<HeightMapRawImage> m_pHeightMapRawImage;
	std::vector<std::unique_ptr<TerrainComponent>> m_pTerrainComponents;
	Vector3 m_v3TerrainScale;

	const static std::string g_strTerrainPath;
};

