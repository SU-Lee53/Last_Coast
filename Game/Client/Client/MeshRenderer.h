#pragma once
#include "Component.h"
#include "Mesh.h"
#include "Material.h"

class GameObject;

class MeshRenderer : public std::enable_shared_from_this<MeshRenderer> {
public:
	UINT m_eObjectRenderType = -1;
	friend struct std::hash<MeshRenderer>;

public:
	MeshRenderer();
	MeshRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials);
	MeshRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~MeshRenderer() {}

	MeshRenderer(const MeshRenderer& other);
	MeshRenderer(MeshRenderer&& other);
	
	MeshRenderer& operator=(const MeshRenderer& other);
	MeshRenderer& operator=(MeshRenderer&& other);

public:
	const std::shared_ptr<Mesh>& GetMesh() const { return m_pMesh; }
	const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_pMaterials; };

	void SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType);

public:
	virtual void Initialize();
	virtual void Update(std::shared_ptr<GameObject> pOwner);
	
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase) {};


public:
	bool operator==(const MeshRenderer& rhs) const;

protected:
	std::shared_ptr<Mesh> m_pMesh = nullptr;
	std::vector<std::shared_ptr<Material>> m_pMaterials;
	bool m_bAddToRenderManager = true;	// 독립적인 렌더링 방법을 가지는 경우 false

};

template<>
struct std::hash<MeshRenderer> {
	size_t operator()(const MeshRenderer& meshRenderer) const {
		return (std::hash<decltype(meshRenderer.m_pMesh)>{}(meshRenderer.m_pMesh) ^
			std::hash<decltype(meshRenderer.m_pMaterials)::value_type>{}(meshRenderer.m_pMaterials[0]));
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DiffusedRenderer
// Position + 정점 색상

class DiffusedRenderer : public MeshRenderer {
public:
	DiffusedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials);
	DiffusedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~DiffusedRenderer() {}


public:
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase) override;


};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedRenderer
// Texture (조명, 노말맵 X)

class TexturedRenderer : public MeshRenderer {
public:
	TexturedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials);
	TexturedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~TexturedRenderer() {}


public:
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase) override;


};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedIlluminatedRenderer
// Texture + 조명

class TexturedIlluminatedRenderer : public MeshRenderer {
public:
	TexturedIlluminatedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials);
	TexturedIlluminatedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~TexturedIlluminatedRenderer() {}


public:
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase) override;


};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalRenderer
// Texture + 조명 + Normal Mapping

class TexturedNormalRenderer : public MeshRenderer {
public:
	TexturedNormalRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials);
	TexturedNormalRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~TexturedNormalRenderer() {}


public:
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase) override;


};



// 2025.12.27
// TODO : 위 MeshRenderer 를 통합하는 Renderer 객체가 필요함