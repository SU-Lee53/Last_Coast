#pragma once
#include "Mesh.h"
#include "Material.h"

class IGameObject;

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

public:
	const std::vector<std::shared_ptr<IMesh>>& GetMeshes() const { return m_pMeshes; }
	const std::vector<MaterialHandle>& GetMaterialHandles() const { return m_MaterialIDs; };
	const MaterialHandle& GetMaterialHandle(size_t idx) const { return m_MaterialIDs[idx]; };
	BoundingOrientedBox GetOBBMerged() const;

	MeshRenderer::ID GetID() const { return m_RuntimeID; }
	MESH_TYPE GetMeshType() { return m_eMeshType; }

	void SetTexture(const TextureRef<Texture>& texHandle, UINT nMaterialIndex, TEXTURE_TYPE eTextureType);

	virtual void ShowControlImGui() override;

protected:
	std::vector<std::shared_ptr<IMesh>> m_pMeshes;
	std::vector<MaterialHandle> m_MaterialIDs;

	MeshRenderer::ID m_RuntimeID = 0;	// ID 가 같다 -> 인스턴싱이 가능하다
	MESH_TYPE m_eMeshType = MESH_TYPE::UNDEFINED;

protected:
	static MeshRenderer::ID g_ui64RendererIDBase;
};

template <>
struct ComponentIndex<MeshRenderer> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::MESH_RENDERER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::MESH_RENDERER);
};

