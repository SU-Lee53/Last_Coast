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

//////////////////////////////////////////////////////////////////////////////////
// DiffusedMaterial

DiffusedMaterial::DiffusedMaterial(const MATERIALLOADINFO& materialLoadInfo)
	:Material(materialLoadInfo)
{
	m_pShader = SHADER->Get<DiffusedShader>();
}

DiffusedMaterial::~DiffusedMaterial()
{
}

void DiffusedMaterial::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, void* dataForBind)
{
}

void DiffusedMaterial::UpdateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
}

//////////////////////////////////////////////////////////////////////////////////
// TexturedMaterial

TexturedMaterial::TexturedMaterial(const MATERIALLOADINFO& materialLoadInfo)
	:Material(materialLoadInfo)
{
	m_pTextures.resize(1);
	m_pTextures[0] = TEXTURE->CreateTextureFromFile(::StringToWString(materialLoadInfo.strAlbedoMapName));	// Diffused
	m_pShader = SHADER->Get<TexturedShader>();
}

TexturedMaterial::~TexturedMaterial()
{
}

void TexturedMaterial::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, void* dataForBind)
{
}

void TexturedMaterial::UpdateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTextures[0]->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.ptr += D3DCore::g_nCBVSRVDescriptorIncrementSize;

	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_OBJ_TEXTURES, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
}

//////////////////////////////////////////////////////////////////////////////////
// TexturedNormalMaterial

TexturedNormalMaterial::TexturedNormalMaterial(const MATERIALLOADINFO& materialLoadInfo)
	:Material(materialLoadInfo)
{
	m_pTextures.resize(2);
	m_pTextures[0] = TEXTURE->CreateTextureFromFile(::StringToWString(materialLoadInfo.strAlbedoMapName));	// Diffused
	m_pTextures[1] = TEXTURE->CreateTextureFromFile(::StringToWString(materialLoadInfo.strNormalMapName));	// Normal
	m_pShader = SHADER->Get<TexturedNormalShader>();
}

TexturedNormalMaterial::~TexturedNormalMaterial()
{
}

void TexturedNormalMaterial::UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, void* dataForBind)
{
}

void TexturedNormalMaterial::UpdateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTextures[0]->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, m_pTextures[1]->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_OBJ_TEXTURES, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
}
