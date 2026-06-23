#pragma once
#include "StaticObject.h"
#include "TerrainComponent.h"
#include "HeightMapRawImage.h"
#include "Mesh.h"

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

class TerrainObject : public StaticObject {
public:
	const auto& GetTerrainComponents() const { return m_pTerrainComponents; }

	float GetHeightWorld(const Vector3& v3WorldPos);
	Vector3 GetNormalWorld(const Vector3& v3WorldPos);

	bool GetHeightNormalWorld(const Vector3& v3WorldPos, OUT float& outfHeight, OUT Vector3& outv3Normal);
	CB_TERRAIN_LAYER_DATA MakeLayerCBData();
	const Vector3& GetTerrainScale() const { return m_v3TerrainScale; }
	TextureRef<Texture> GetHeightMapTexture() const { return (m_pHeightMapRawImage) ? m_pHeightMapRawImage->GetHeightMapTexture() : TextureRef<Texture>{}; }
	const std::shared_ptr<TerrainQuadMesh>& GetTerrainQuadMesh() const { return m_pTerrainQuadMesh; }

	float GetMinHeight() const { return m_fMinTerrainHeight; };
	float GetMaxHeight() const { return m_fMaxTerrainHeight; };

	std::vector<std::shared_ptr<TerrainComponent>> GetTerrainComponentsInFrustum(const BoundingFrustum& xmFrustumWorld) const;


public:
	HRESULT LoadFromFiles(const std::string& strFilename);

private:
	void BuildTerrainMesh(const TERRAINLOADINFO& terrainInfo);
	void ReadTerrainData(const nlohmann::json& j, OUT TERRAINLOADINFO& outTerrainInfo);
	TERRAINCOMPONENTLOADINFO ReadTerrainComponentData(const nlohmann::json& j);
	TERRAINLAYERLOADINFO ReadTerrainLayerData(const nlohmann::json& j);

private:
	std::unique_ptr<HeightMapRawImage> m_pHeightMapRawImage;
	std::vector<std::shared_ptr<TerrainComponent>> m_pTerrainComponents;
	std::shared_ptr<TerrainQuadMesh> m_pTerrainQuadMesh;
	Vector3 m_v3TerrainScale;

	float m_fMinTerrainHeight = std::numeric_limits<float>::max();
	float m_fMaxTerrainHeight = -std::numeric_limits<float>::max();

	const static std::string g_strTerrainPath;

};

