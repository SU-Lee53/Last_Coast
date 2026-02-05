#include "pch.h"
#include "MeshRenderer.h"

uint64 MeshRenderer::g_ui64RendererIDBase = 0;

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
	m_ui64RendererID = ++g_ui64RendererIDBase;
}

MeshRenderer::MeshRenderer(std::shared_ptr<IGameObject> pOwner, const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
	: IComponent{ pOwner }
{
	m_pMeshes.reserve(meshLoadInfos.size());

	for (const auto& meshLoadInfo : meshLoadInfos) {
		std::shared_ptr<Mesh> pMesh;
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

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<Material> pMaterial;
		switch (m_eMeshType)
		{
		case MESH_TYPE::STATIC:
		{
			pMaterial = std::make_shared<StandardMaterial>(materialInfo);
			break;
		}
		case MESH_TYPE::SKINNED:
		{
			pMaterial = std::make_shared<SkinnedMaterial>(materialInfo);
			break;
		}
		case MESH_TYPE::TERRAIN:
		{
			pMaterial = std::make_shared<TerrainMaterial>(materialInfo);
			break;
		}
		default:
			std::unreachable();
		}
		m_pMaterials.push_back(pMaterial);
	}

	m_ui64RendererID = ++g_ui64RendererIDBase;
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
	pClone->m_pMeshes = m_pMeshes;
	pClone->m_pMaterials = m_pMaterials;
	pClone->m_ui64RendererID = m_ui64RendererID;
	pClone->m_eRenderType = m_eRenderType;
	pClone->m_eMeshType = m_eMeshType;
	pClone->SetOwner(pNewOwner);

	return pClone;
}


void MeshRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
	DescriptorHandle& descHandle, int32 nInstanceCount, OUT int32& outnInstanceBase, const Matrix& mtxWorld) const
{
	for (int i = 0; i < m_pMeshes.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), outnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		if (m_eMeshType == MESH_TYPE::STATIC) {
			pd3dDevice->CopyDescriptorsSimple(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::OBJ_MATERIAL_DATA), descHandle.gpuHandle);
			descHandle.cpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}
		else {
			Matrix mtxWorldTransposed = mtxWorld.Transpose();
			ConstantBuffer worldCBuffer = RESOURCE->AllocCBuffer<CB_WORLD_TRANSFORM_DATA>();
			worldCBuffer.WriteData(&mtxWorldTransposed);

			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, worldCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

			pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::OBJ_MATERIAL_DATA), descHandle.gpuHandle);
			descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}

		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMeshes[i]->Render(pd3dCommandList, nInstanceCount);
	}
	outnInstanceBase += nInstanceCount;
}

void MeshRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
	DescriptorHandle& descHandle, int32 unStartIndex, int32 unIndexCount, int32 nInstanceCount, OUT int32& outnInstanceBase, const Matrix& mtxWorld) const
{
	for (int i = 0; i < m_pMeshes.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), outnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		if (m_eMeshType == MESH_TYPE::STATIC) {
			pd3dDevice->CopyDescriptorsSimple(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::OBJ_MATERIAL_DATA), descHandle.gpuHandle);
			descHandle.cpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}
		else {
			Matrix mtxWorldTransposed = mtxWorld.Transpose();
			ConstantBuffer worldCBuffer = RESOURCE->AllocCBuffer<CB_WORLD_TRANSFORM_DATA>();
			worldCBuffer.WriteData(&mtxWorldTransposed);

			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, worldCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

			pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::OBJ_MATERIAL_DATA), descHandle.gpuHandle);
			descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}

		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMeshes[i]->Render(pd3dCommandList, unStartIndex, unIndexCount, nInstanceCount);
	}
	outnInstanceBase += nInstanceCount;
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

void MeshRenderer::SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType)
{
	assert(pTexture);
	m_pMaterials[nMaterialIndex]->SetTexture(pTexture, eTextureType);
}
