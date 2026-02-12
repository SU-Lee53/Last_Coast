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
	using ID = uint64;

public:
	MeshRenderer(std::shared_ptr<IGameObject> pOwner);
	MeshRenderer(std::shared_ptr<IGameObject> pOwner, const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~MeshRenderer() {}

public:
	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

	void Render(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		DescriptorHandle& descHandle, 
		int32 unStartIndex, 
		int32 unIndexCount, 
		int32 nInstanceCount, 
		OUT int32& outnInstanceBase, 
		const Matrix& mtxWorld = Matrix::Identity) const;

public:
	const std::vector<std::shared_ptr<IMesh>>& GetMeshes() const { return m_pMeshes; }
	const std::vector<IMaterial::ID>& GetMaterialIDs() const { return m_MaterialIDs; };
	BoundingOrientedBox GetOBBMerged() const;

	MeshRenderer::ID GetID() const { return m_RuntimeID; }
	OBJECT_RENDER_TYPE GetRenderType() const { return m_eRenderType; }
	MESH_TYPE GetMeshType() { return m_eMeshType; }

	void SetTexture(Texture::ID texID, UINT nMaterialIndex, TEXTURE_TYPE eTextureType);

protected:
	std::vector<std::shared_ptr<IMesh>> m_pMeshes;
	std::vector<IMaterial::ID> m_MaterialIDs;

	MeshRenderer::ID m_RuntimeID = 0;	// ID 가 같다 -> 인스턴싱이 가능하다
	OBJECT_RENDER_TYPE m_eRenderType = OBJECT_RENDER_TYPE::FORWARD;
	MESH_TYPE m_eMeshType = MESH_TYPE::UNDEFINED;

protected:
	static MeshRenderer::ID g_ui64RendererIDBase;
};

template <>
struct ComponentIndex<MeshRenderer> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::MESH_RENDERER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::MESH_RENDERER);
};
