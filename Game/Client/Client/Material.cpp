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

void IMaterial::SetTexture(const TextureRef<Texture>& texHandle, TEXTURE_TYPE eTextureType)
{
	if (m_TextureIDs.size() < std::to_underlying(eTextureType) + 1) {
		m_TextureIDs.resize(std::to_underlying(eTextureType) + 1);
	}
	m_TextureIDs[std::to_underlying(eTextureType)] = texHandle;
}

std::shared_ptr<Texture> IMaterial::GetTexture(int nIndex)
{
	assert(nIndex < m_TextureIDs.size());

	return m_TextureIDs[nIndex].GetResource();
}

void IMaterial::ShowControlToImGui()
{
	ImGui::Text("%s : (%f, %f, %f, %f)", "v4Ambient", m_MaterialData.v4Ambient.x, m_MaterialData.v4Ambient.x, m_MaterialData.v4Ambient.y, m_MaterialData.v4Ambient.w);
	ImGui::Text("%s : (%f, %f, %f, %f)", "v4Diffuse", m_MaterialData.v4Diffuse.x, m_MaterialData.v4Diffuse.x, m_MaterialData.v4Diffuse.y, m_MaterialData.v4Diffuse.w);
	ImGui::Text("%s : (%f, %f, %f, %f)", "v4Specular", m_MaterialData.v4Specular.x, m_MaterialData.v4Specular.x, m_MaterialData.v4Specular.y, m_MaterialData.v4Specular.w);
	ImGui::Text("%s : (%f, %f, %f, %f)", "v4Emissive", m_MaterialData.v4Emissive.x, m_MaterialData.v4Emissive.x, m_MaterialData.v4Emissive.y, m_MaterialData.v4Emissive.w);

	ImGui::Text("%s : %f", "fGlossiness", m_MaterialData.fGlossiness);
	ImGui::Text("%s : %f", "fSmoothness", m_MaterialData.fSmoothness);
	ImGui::Text("%s : %f", "fSpecularHighlight", m_MaterialData.fSpecularHighlight);
	ImGui::Text("%s : %f", "fMetallic", m_MaterialData.fMetallic);
	ImGui::Text("%s : %f", "fGlossyReflection", m_MaterialData.fGlossyReflection);

	int nCnt{};
	for (const auto& texRef : m_TextureIDs) {
		std::string strTreeName = std::format("Texture #{}", nCnt++);
		if (ImGui::TreeNode(strTreeName.c_str())) {
			auto pTex = texRef.GetResource();
			(pTex) ? pTex->ShowDebugInfo() : ImGui::Text("nullptr");
			ImGui::TreePop();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

void StandardMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, !materialLoadInfo.bHasAlbedoAlphaMode);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName, false);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName, false);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName, false);		// Specular

	bool bLoadedMaterialAlbedo = m_TextureIDs[0].IsValid();
	if (!m_TextureIDs[0].IsValid()) {
		m_TextureIDs[0] = TEXTURE->LoadTexture("DefaultMaterial_BaseColor_0");
		if (!m_TextureIDs[1].IsValid()) {
			m_TextureIDs[1] = TEXTURE->LoadTexture("DefaultMaterial_Normal_0");
		}
	}

	if (m_TextureIDs[0].IsValid()) {
		if (bLoadedMaterialAlbedo && materialLoadInfo.bHasAlbedoAlphaMode) {
			Texture::ALPHA_MODE eAlphaMode = static_cast<Texture::ALPHA_MODE>(materialLoadInfo.unAlbedoAlphaMode);
			m_TextureIDs[0].GetResource()->SetAlphaMode(eAlphaMode);
			m_MaterialData.eAlphaMode = materialLoadInfo.unAlbedoAlphaMode;
		}
		else {
			m_MaterialData.eAlphaMode = std::to_underlying(m_TextureIDs[0].GetResource()->GetAlphaMode());
		}
	}

	m_pShader = SHADER->Get<StandardShader>();
}

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

void SkinnedMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	m_TextureIDs.resize(4);
	m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, !materialLoadInfo.bHasAlbedoAlphaMode);			// Diffused
	m_TextureIDs[1] = TEXTURE->LoadTexture(materialLoadInfo.strNormalMapName, false);			// Normal
	m_TextureIDs[2] = TEXTURE->LoadTexture(materialLoadInfo.strMetallicMapName, false);		// Metallic
	m_TextureIDs[3] = TEXTURE->LoadTexture(materialLoadInfo.strSpecularMapName, false);		// Specular

	bool bLoadedMaterialAlbedo = m_TextureIDs[0].IsValid();
	if (!m_TextureIDs[0].IsValid()) {
		m_TextureIDs[0] = TEXTURE->LoadTexture("DefaultMaterial_BaseColor_0");
		if (!m_TextureIDs[1].IsValid()) {
			m_TextureIDs[1] = TEXTURE->LoadTexture("DefaultMaterial_Normal_0");
		}
	}

	if (m_TextureIDs[0].IsValid()) {
		if (bLoadedMaterialAlbedo && materialLoadInfo.bHasAlbedoAlphaMode) {
			Texture::ALPHA_MODE eAlphaMode = static_cast<Texture::ALPHA_MODE>(materialLoadInfo.unAlbedoAlphaMode);
			m_TextureIDs[0].GetResource()->SetAlphaMode(eAlphaMode);
			m_MaterialData.eAlphaMode = materialLoadInfo.unAlbedoAlphaMode;
		}
		else {
			m_MaterialData.eAlphaMode = std::to_underlying(m_TextureIDs[0].GetResource()->GetAlphaMode());
		}
	}

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

//////////////////////////////////////////////////////////////////////////////////
// WaterMaterial

void WaterMaterial::Initialize(const MATERIALLOADINFO& materialLoadInfo)
{
	InitializeColors(materialLoadInfo);

	if (m_MaterialData.v4Diffuse == Vector4(0.0f, 0.0f, 0.0f, 0.0f)) {
		m_MaterialData.v4Diffuse = Vector4(0.02f, 0.20f, 0.32f, 1.0f);
	}
	if (m_MaterialData.v4Specular == Vector4(0.0f, 0.0f, 0.0f, 0.0f)) {
		m_MaterialData.v4Specular = Vector4(1.0f, 1.0f, 1.0f, 0.65f);
	}
	m_MaterialData.fSmoothness = (m_MaterialData.fSmoothness <= 0.0f) ? 0.86f : m_MaterialData.fSmoothness;
	m_MaterialData.fMetallic = 0.0f;
	m_MaterialData.eAlphaMode = std::to_underlying(Texture::ALPHA_MODE::Opaque);

	m_TextureIDs.resize(4);
	if (!materialLoadInfo.strAlbedoMapName.empty() && materialLoadInfo.strAlbedoMapName != "None") {
		m_TextureIDs[0] = TEXTURE->LoadTexture(materialLoadInfo.strAlbedoMapName, false);
	}

	m_pShader = SHADER->Get<WaterShader>();
}
