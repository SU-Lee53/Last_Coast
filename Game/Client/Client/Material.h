#pragma once
#include "Texture.h"
#include "Shader.h"

struct MATERIALLOADINFO {
	Vector4			v4Ambient;
	Vector4			v4Diffuse;
	Vector4			v4Specular; //(r,g,b,a=power)
	Vector4			v4Emissive;

	float			fGlossiness = 0.0f;
	float			fSmoothness = 0.0f;
	float			fSpecularHighlight = 0.0f;
	float			fMetallic = 0.0f;
	float			fGlossyReflection = 0.0f;

	UINT			eType = 0x00;

	std::string		strAlbedoMapName;
	std::string		strSpecularMapName;
	std::string		strMetallicMapName;
	std::string		strNormalMapName;
	std::string		strEmissionMapName;
	std::string		strDetailAlbedoMapName;
	std::string		strDetailNormalMapName;

	// Terrain material data
	std::string strTerrainLayerName;
	uint32 unTerrainLayerIndex;
	float fUVTiling;
}; 

interface IMaterial abstract {
public:
	friend class MaterialManager;
	using ID = uint64;

public:
	const MaterialColors& GetMaterialColors() const { return m_MaterialColors; }
	const std::shared_ptr<Shader>& GetShader() const { return m_pShader; }
	void SetShader(std::shared_ptr<Shader> pShader);

	void SetTexture(Texture::ID texID, TEXTURE_TYPE eTextureType);
	std::shared_ptr<Texture> GetTexture(int nIndex);

protected:
	IMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~IMaterial();

protected:
	MaterialColors m_MaterialColors{};
	std::vector<Texture::ID> m_TextureIDs;

	std::shared_ptr<Shader> m_pShader;
};

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

class StandardMaterial : public IMaterial {
private:
	StandardMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~StandardMaterial() {}

};

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

class SkinnedMaterial : public IMaterial {
private:
	SkinnedMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~SkinnedMaterial() {}

};

//////////////////////////////////////////////////////////////////////////////////
// TerrainMaterial

class TerrainMaterial : public IMaterial {
private:
	TerrainMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~TerrainMaterial() {}

	float GetTiling() const { return m_fTiling; }

private:
	std::string m_strLayerName;
	uint32 m_unIndex;
	float m_fTiling;
};

