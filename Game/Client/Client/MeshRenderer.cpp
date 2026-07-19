#include "pch.h"
#include "MeshRenderer.h"

std::atomic_uint64_t MeshRenderer::g_ui64RendererIDBase = 0;

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
	m_RuntimeID = g_ui64RendererIDBase.fetch_add(1) + 1;
}

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner, const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo, const std::string& strMaterialKeyPrefix)
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
		case MESH_TYPE::WATER:
		{
			pMesh = std::make_shared<StaticMesh>(meshLoadInfo);
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
		std::string strMaterialKey = strMaterialKeyPrefix + pOwner->GetName() + std::to_string(unCount++);
		MaterialHandle handle{};
		switch (m_eMeshType)
		{
		case MESH_TYPE::STATIC:
		{
			handle = MATERIAL->LoadMaterial<StandardMaterial>(strMaterialKey, materialInfo);
			break;
		}
		case MESH_TYPE::SKINNED:
		{
			handle = MATERIAL->LoadMaterial<SkinnedMaterial>(strMaterialKey, materialInfo);
			break;
		}
		case MESH_TYPE::TERRAIN:
		{
			handle = MATERIAL->LoadMaterial<TerrainMaterial>(strMaterialKey, materialInfo);
			break;
		}
		case MESH_TYPE::WATER:
		{
			handle = MATERIAL->LoadMaterial<WaterMaterial>(strMaterialKey, materialInfo);
			break;
		}
		default:
			std::unreachable();
		}

		if (!handle.IsValid()) {
			OutputDebugStringA("Material load failed");
			continue;
		}

		m_MaterialIDs.push_back(handle);
	}

	m_RuntimeID = g_ui64RendererIDBase.fetch_add(1) + 1;
}

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner, CloneTag)
	: IComponent{ pOwner }
{
}

void MeshRenderer::Initialize()
{
	m_bInitialized = true;
}

void MeshRenderer::Update()
{
}

std::shared_ptr<IComponent> MeshRenderer::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	auto pClone = std::shared_ptr<MeshRenderer>(new MeshRenderer(pNewOwner, CloneTag{}));

	pClone->m_pMeshes = m_pMeshes;
	pClone->m_MaterialIDs = m_MaterialIDs;
	pClone->m_RuntimeID = m_RuntimeID;	// 반드시 필수
	pClone->m_eMeshType = m_eMeshType;

	return pClone;
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

void MeshRenderer::SetTexture(const TextureRef<Texture>& texHandle, UINT nMaterialIndex, TEXTURE_TYPE eTextureType)
{
	assert(texHandle.IsValid());
	auto pMaterial = m_MaterialIDs[nMaterialIndex].GetResource();
	pMaterial->SetTexture(texHandle, eTextureType);
}

void MeshRenderer::ShowControlImGui()
{
	int nCnt{};
	for (auto [pMesh, mat] :std::views::zip(m_pMeshes, m_MaterialIDs)) {
		std::string strTreeName = std::format("Mesh - Material #{}", nCnt++);
		if (ImGui::TreeNode(strTreeName.c_str())) {
			ImGui::SeparatorText("Mesh");
			pMesh->ShowControlImGui();

			ImGui::SeparatorText("Material");
			mat.GetResource()->ShowControlToImGui();
			ImGui::TreePop();
		}
	}
}

