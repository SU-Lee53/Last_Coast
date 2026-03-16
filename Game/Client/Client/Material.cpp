#include "pch.h"
#include "Material.h"

void IMaterial::InitializeColors(const MATERIALLOADINFO& materialLoadInfo)
{
	m_MaterialData.v4Ambient = materialLoadInfo.v4Ambient;
	m_MaterialData.v4Diffuse = materialLoadInfo.v4Diffuse;
	m_MaterialData.v4Specular = materialLoadInfo.v4Specular;
	m_MaterialData.v4Emissive = materialLoadInfo.v4Emissive;

	m_MaterialData.fGlossiness = materialLoadInfo.fGlossiness;
	m_MaterialData.fSmoothness = materialLoadInfo.fSmoothness;
	m_MaterialData.fSpecularHighlight = materialLoadInfo.fSpecularHighlight;
	m_MaterialData.fMetallic = materialLoadInfo.fMetallic;
	m_MaterialData.fGlossyReflection = materialLoadInfo.fGlossyReflection;
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
	assert(nIndex < m_TextureIDs.size());

	return TEXTURE->GetTextureByID(m_TextureIDs[nIndex], TEXTURE_RESOURCE_TYPE::SRV);
}

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

void StandardMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, true);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName, false);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName, false);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName, false);		// Specular
	m_pShader = SHADER->Get<StandardShader>();
}

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

void SkinnedMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, true);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName, false);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName, false);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName, false);		// Specular
	m_pShader = SHADER->Get<AnimatedShader>();
}

//////////////////////////////////////////////////////////////////////////////////
// TerrainMaterial

void TerrainMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	m_strLayerName = materialLoadInfo.strTerrainLayerName;
	m_unIndex = materialLoadInfo.unTerrainLayerIndex;
	m_fTiling = materialLoadInfo.fUVTiling;

	m_TextureIDs.resize(2);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, false);		// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName, false);		// Normal

	m_pShader = SHADER->Get<TerrainShader>();
}
