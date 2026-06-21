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
	virtual void Initialize(const MATERIALLOADINFO& materialLoadInfo) = 0;

	bool HasAlphaMask() const { return (m_MaterialData.eAlphaMode == std::to_underlying(Texture::ALPHA_MODE::Masked)); }
	const MaterialData& GetMaterialData() const { return m_MaterialData; }
	const std::shared_ptr<Shader>& GetShader() const { return m_pShader; }
	void SetShader(std::shared_ptr<Shader> pShader);

	void SetTexture(const TextureRef<Texture>& texID, TEXTURE_TYPE eTextureType);
	std::shared_ptr<Texture> GetTexture(int nIndex);

	const std::vector<TextureRef<Texture>>& GetTextureRefs() const { return m_TextureIDs; }

	virtual void ShowControlToImGui();

protected:
	void InitializeColors(const MATERIALLOADINFO& materialLoadInfo);

protected:
	MaterialData m_MaterialData{};
	std::vector<TextureRef<Texture>> m_TextureIDs;

	std::shared_ptr<Shader> m_pShader;
};

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

class StandardMaterial : public IMaterial {
public:
	virtual void Initialize(const MATERIALLOADINFO& materialLoadInfo) override;

};

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

class SkinnedMaterial : public IMaterial {
public:
	virtual void Initialize(const MATERIALLOADINFO& materialLoadInfo) override;

};

//////////////////////////////////////////////////////////////////////////////////
// TerrainMaterial

class TerrainMaterial : public IMaterial {
public:
	virtual void Initialize(const MATERIALLOADINFO& materialLoadInfo) override;
	float GetTiling() const { return m_fTiling; }

private:
	std::string m_strLayerName;
	uint32 m_unIndex;
	float m_fTiling;
};

//////////////////////////////////////////////////////////////////////////////////
// WaterMaterial

class WaterMaterial : public IMaterial {
public:
	virtual void Initialize(const MATERIALLOADINFO& materialLoadInfo) override;
};
