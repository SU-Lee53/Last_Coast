#include "pch.h"
#include "Material.h"

Material::Material(const MATERIALLOADINFO& materialLoadInfo)
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

Material::~Material()
{
}

void Material::SetShader(std::shared_ptr<Shader> pShader)
{
	m_pShader = pShader;
}

void Material::SetTexture(std::shared_ptr<Texture> pTexture, TEXTURE_TYPE eTextureType)
{
	if (m_pTextures.size() < eTextureType + 1) {
		m_pTextures.resize(eTextureType + 1);
	}
	m_pTextures[eTextureType] = pTexture;
}

std::shared_ptr<Texture> Material::GetTexture(int nIndex)
{
	assert(nIndex < m_pTextures.size());
	return m_pTextures[nIndex];
}

void Material::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, void* dataForBind)
{
}

void Material::UpdateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	for (int i = 0; i < m_pTextures.size(); ++i) {
		if (m_pTextures[i]) {
			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTextures[i]->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::OBJ_TEXTURES), descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(4, D3DCore::g_nCBVSRVDescriptorIncrementSize);
}

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

StandardMaterial::StandardMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: Material(materialLoadInfo)
{
	m_pTextures.resize(4);
	m_pTextures[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);		// Diffused
	m_pTextures[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);		// Normal
	m_pTextures[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName);		// Metallic
	m_pTextures[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName);		// Specular
	m_pShader = SHADER->Get<StandardShader>();
}

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

SkinnedMaterial::SkinnedMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: Material(materialLoadInfo)
{
	m_pTextures.resize(4);
	m_pTextures[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);		// Diffused
	m_pTextures[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);		// Normal
	m_pTextures[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName);		// Metallic
	m_pTextures[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName);		// Specular
	m_pShader = SHADER->Get<AnimatedShader>();
}

TerrainMaterial::TerrainMaterial(const MATERIALLOADINFO& materialLoadInfo)
	: Material(materialLoadInfo)
{
	m_strLayerName = materialLoadInfo.strTerrainLayerName;
	m_unIndex = materialLoadInfo.unTerrainLayerIndex;
	m_fTiling = materialLoadInfo.fUVTiling;

	m_pTextures.resize(2);
	m_pTextures[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName);		// Diffused
	m_pTextures[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName);		// Normal

	m_pShader = SHADER->Get<TerrainShader>();
}

