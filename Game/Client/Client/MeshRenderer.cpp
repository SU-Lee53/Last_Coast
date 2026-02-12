#include "pch.h"
#include "MeshRenderer.h"

MeshRenderer::ID MeshRenderer::g_ui64RendererIDBase = 0;

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
	m_RuntimeID = ++g_ui64RendererIDBase;
}

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner, const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
	: IComponent{ pOwner }
{
	m_pMeshes.reserve(meshLoadInfos.size());

	for (const auto& meshLoadInfo : meshLoadInfos) {
		std::shared_ptr<IMesh> pMesh;
		switch (meshLoadInfo.eMeshType)
		{
		case MESH_TYPE::STATIC:
		{
			pMesh = std::make_shared<StaticMesh>(meshLoadInfo);
			break;
		}
		case MESH_TYPE::SKINNED:
		{
			pMesh = std::make_shared<SkinnedMesh>(meshLoadInfo);
			break;
		}
		case MESH_TYPE::TERRAIN:
		{
			pMesh = std::make_shared<TerrainMesh>(meshLoadInfo);
			break;
		}
		default:
			std::unreachable();
		}

		m_eMeshType = meshLoadInfo.eMeshType;
		m_pMeshes.push_back(pMesh);
	}

	uint32 unCount = 0;
	for (const auto& materialInfo : materialLoadInfo) {
		std::string strMaterialKey = pOwner->GetName() + std::to_string(unCount++);
		IMaterial::ID id{};
		switch (m_eMeshType)
		{
		case MESH_TYPE::STATIC:
		{
			id = MATERIAL->LoadMaterial<StandardMaterial>(strMaterialKey, materialInfo);
			break;
		}
		case MESH_TYPE::SKINNED:
		{
			id = MATERIAL->LoadMaterial<SkinnedMaterial>(strMaterialKey, materialInfo);
			break;
		}
		case MESH_TYPE::TERRAIN:
		{
			id = MATERIAL->LoadMaterial<TerrainMaterial>(strMaterialKey, materialInfo);
			break;
		}
		default:
			std::unreachable();
		}

		if (id == INVALID_ID) {
			OutputDebugStringA("Material load failed");
			continue;
		}

		m_MaterialIDs.push_back(id);
	}

	m_RuntimeID = ++g_ui64RendererIDBase;
}

void MeshRenderer::Initialize()
{
	m_eRenderType = OBJECT_RENDER_TYPE::FORWARD;
	m_bInitialized = true;
}

void MeshRenderer::Update()
{
	if (m_eMeshType == MESH_TYPE::TERRAIN || m_eMeshType == MESH_TYPE::SKINNED) {
		return;
	}

	MeshRenderParameters meshParam{
		.mtxWorld = m_wpOwner.lock()->GetWorldMatrix().Transpose()
	};
#ifdef WITH_FRUSTUM_CULLING
	const auto& pCamera = CUR_SCENE->GetCamera();
	const auto& xmFrustumInWorld = pCamera->GetFrustumWorld();
	const Matrix& mtxWorld = GetOwner()->GetWorldMatrix().Invert();
	for (const auto& pMesh : m_pMeshes) {
		BoundingFrustum xmFrustum;
		xmFrustumInWorld.Transform(xmFrustum, mtxWorld);
		if (xmFrustum.Intersects(pMesh->GetBoundingBox())) {
			RENDER->Add(std::static_pointer_cast<MeshRenderer>(shared_from_this()), meshParam);
		}
	}
#else
	for (int i = 0; i < m_pMeshes.size(); ++i) {
		RENDER->Add(std::static_pointer_cast<MeshRenderer>(shared_from_this()), meshParam);
	}
#endif
}

std::shared_ptr<IComponent> MeshRenderer::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<MeshRenderer> pClone = std::make_shared<MeshRenderer>(pNewOwner);
	g_ui64RendererIDBase--; // 의도치 않게 올라간 RuntimeIDBase 를 다시 하나 내려주어야 함 (필수는 아니고 디버깅이 편함)

	pClone->m_pMeshes = m_pMeshes;
	pClone->m_MaterialIDs = m_MaterialIDs;
	pClone->m_RuntimeID = m_RuntimeID;	// 반드시 필수
	pClone->m_eRenderType = m_eRenderType;
	pClone->m_eMeshType = m_eMeshType;

	return pClone;
}

void MeshRenderer::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int32 unStartIndex, int32 unIndexCount, int32 nInstanceCount, OUT int32& outnInstanceBase, const Matrix& mtxWorld) const
{
	// TODO : 구현
}

BoundingOrientedBox MeshRenderer::GetOBBMerged() const
{
	BoundingBox xmAABBMerged;
	for (const auto& pMesh : m_pMeshes) {
		XMFLOAT3 pxmf3OBBPoints[BoundingOrientedBox::CORNER_COUNT];
		pMesh->GetBoundingBox().GetCorners(pxmf3OBBPoints);
		BoundingBox xmAABB{};
		BoundingBox::CreateFromPoints(xmAABB, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints, sizeof(XMFLOAT3));

		if (xmAABBMerged.Center == Vector3(0, 0, 0) && xmAABBMerged.Extents == Vector3(1, 1, 1)) {
			xmAABBMerged = xmAABB;
		}
		else {
			BoundingBox::CreateMerged(xmAABBMerged, xmAABBMerged, xmAABB);
		}
	}

	BoundingOrientedBox xmOBBResult{};
	BoundingOrientedBox::CreateFromBoundingBox(xmOBBResult, xmAABBMerged);

	return xmOBBResult;
}

void MeshRenderer::SetTexture(Texture::ID texID, UINT nMaterialIndex, TEXTURE_TYPE eTextureType)
{
	assert(pTexture);
	auto pMaterial = MATERIAL->GetMaterialByID(m_MaterialIDs[nMaterialIndex]);
	pMaterial->SetTexture(texID, eTextureType);
}
