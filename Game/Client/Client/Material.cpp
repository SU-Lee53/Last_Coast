#include "pch.h"
#include "Material.h"

IMaterial::IMaterial(const MATERIALLOADINFO& materialLoadInfo)
{
	m_MaterialColors.xmf4Ambient = materialLoadInfo.v4Ambient;
	m_MaterialColors.xmf4Diffuse = materialLoadInfo.v4Diffuse;
	m_MaterialColors.xmf4Specular = materialLoadInfo.v4Specular;
	m_MaterialColors.xmf4Emissive = materialLoadInfo.v4Emissive;

	m_MaterialColors.fGlossiness = materialLoadInfo.fGlossiness;
	m_MaterialColors.fSmoothness = materialLoadInfo.fSmoothness;
	m_MaterialColors.fSpecularHighlight = materialLoadInfo.fSpecularHighlight;
	m_MaterialColors.fMetallic = materialLoadInfo.fMetallic;
	m_MaterialColors.fGlossyReflection = materialLoadInfo.fGlossyReflection;
}

IMaterial::~IMaterial()
{
}

void IMaterial::SetShader(std::shared_ptr<Shader> pShader)
{
	m_pShader = pShader;
}

void IMaterial::SetTexture(Texture::ID texID, TEXTURE_TYPE eTextureType)
{
	if (m_TextureIDs.size() < std::to_underlying(eTextureType) + 1) {
		m_TextureIDs.resize(std::to_underlying(eTextureType) + 1);
	}
	m_TextureIDs[std::to_underlying(eTextureType)] = texID;
}

std::shared_ptr<Texture> IMaterial::GetTexture(int nIndex)
{
	assert(nIndex < m_pTextures.size());

	return TEXTURE->GetTextureByID(m_TextureIDs[nIndex]);
}

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

StandardMaterial::StandardMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: IMaterial(materialLoadInfo)
{
	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName);		// Specular
	m_pShader = SHADER->Get<StandardShader>();
}

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

SkinnedMaterial::SkinnedMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: IMaterial(materialLoadInfo)
{
	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName);		// Specular
	m_pShader = SHADER->Get<AnimatedShader>();
}

TerrainMaterial::TerrainMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: IMaterial(materialLoadInfo)
{
	m_strLayerName = materialLoadInfo.strTerrainLayerName;
	m_unIndex = materialLoadInfo.unTerrainLayerIndex;
	m_fTiling = materialLoadInfo.fUVTiling;

	m_TextureIDs.resize(2);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);		// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);		// Normal

	m_pShader = SHADER->Get<TerrainShader>();
}

