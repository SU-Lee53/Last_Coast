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
}; 

class Material {
public:
	Material(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~Material();

public:
	const MaterialColors& GetMaterialColors() const { return m_MaterialColors; }
	const std::shared_ptr<Shader>& GetShader() const { return m_pShader; }
	void SetShader(std::shared_ptr<Shader> pShader);

	void SetTexture(std::shared_ptr<Texture> pTexture, TEXTURE_TYPE eTextureType);
	std::shared_ptr<Texture> GetTexture(int nIndex);

public:
	virtual void UpdateShaderVariables(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, void* dataForBind);
	virtual void UpdateShaderVariables(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle);

protected:
	MaterialColors m_MaterialColors{};
	std::vector<std::shared_ptr<Texture>> m_pTextures;

	std::shared_ptr<Shader> m_pShader;

};

//////////////////////////////////////////////////////////////////////////////////
// StandardMaterial

class StandardMaterial : public Material {
public:
	StandardMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~StandardMaterial() {}

};

//////////////////////////////////////////////////////////////////////////////////
// SkinnedMaterial

class SkinnedMaterial : public Material {
public:
	SkinnedMaterial(const MATERIALLOADINFO& materialLoadInfo);
	virtual ~SkinnedMaterial() {}

};

//////////////////////////////////////////////////////////////////////////////////
// TerrainMaterial

class TerrainMaterial : public Material {
public:
	TerrainMaterial(const MATERIALLOADINFO& materialLoadInfo, const std::string& strLayerName, uint32 unIndex, float fTiling);
	virtual ~TerrainMaterial() {}

private:
	std::string m_strLayerName;
	uint32 m_unIndex;
	float m_fTiling;
};

