#pragma once
#include "Mesh.h"
#include "Material.h"
#include "MeshRenderer.h"

class IGameObject;

enum class OBJECT_RENDER_TYPE : uint8 {
	FORWARD = 0,
	DIFFERED,

	COUNT
};

//#define WITH_FRUSTUM_CULLING

class MeshRenderer : public IComponent {
public:
	MeshRenderer(std::shared_ptr<IGameObject> pOwner);
	MeshRenderer(std::shared_ptr<IGameObject> pOwner, const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~MeshRenderer() {}

public:
	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

	void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		DescriptorHandle& descHandle, int32 nInstanceCount, OUT int32& outnInstanceBase, const Matrix& mtxWorld = Matrix::Identity) const;
	void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		DescriptorHandle& descHandle, int32 unStartIndex, int32 unIndexCount, int32 nInstanceCount, OUT int32& outnInstanceBase, const Matrix& mtxWorld = Matrix::Identity) const;

public:
	const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_pMeshes; }
	const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_pMaterials; };
	BoundingOrientedBox GetOBBMerged() const;

	uint64_t GetID() const { return m_ui64RendererID; }
	OBJECT_RENDER_TYPE GetRenderType() const { return m_eRenderType; }
	MESH_TYPE GetMeshType() { return m_eMeshType; }

	void SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType);

protected:
	std::vector<std::shared_ptr<Mesh>> m_pMeshes;
	std::vector<std::shared_ptr<Material>> m_pMaterials;

	uint64_t m_ui64RendererID = 0;	// ID 가 같다 -> 인스턴싱이 가능하다
	OBJECT_RENDER_TYPE m_eRenderType = OBJECT_RENDER_TYPE::FORWARD;
	MESH_TYPE m_eMeshType = MESH_TYPE::UNDEFINED;

protected:
	static uint64 g_ui64RendererIDBase;


};

template <>
struct ComponentIndex<MeshRenderer> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::MESH_RENDERER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::MESH_RENDERER);
};
