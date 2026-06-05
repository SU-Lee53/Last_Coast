#include "pch.h"
#include "GBufferPass.h"
#include "TerrainObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// GBufferPass

void GBufferPass::Initialize()
{
	//const uint32 unRTVs = 3;
	//for (uint32 i = 0; i < RenderManager::g_unMaxPendingFrames; ++i) {
	//	m_pRTVs[i].reserve(unRTVs);
	//	
	//	for (uint32 j = 0; j < unRTVs; ++j) {
	//		auto& [srvID, rtvID] = TEXTURE->LoadRenderTargetTexture(
	//			"GBuffer_" + std::to_string(i) + "_" + std::to_string(j),
	//			WinCore::g_dwClientWidth,
	//			WinCore::g_dwClientHeight,
	//			DXGI_FORMAT_R8G8B8A8_UNORM,
	//			DXGI_FORMAT_R8G8B8A8_UNORM);
	//
	//		auto pRTV = std::static_pointer_cast<RenderTargetTexture>(TEXTURE->GetTextureByID(rtvID, TEXTURE_RESOURCE_TYPE::RTV));
	//		m_pRTVs[i].push_back(pRTV);
	//	}
	//}
}

void GBufferPass::SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const
{
	// Set Render Targets
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();
	const auto& currentGBuffer = RENDER->GetGBuffer();

	for (auto& gBuffer : currentGBuffer.GBuffers) {
		gBuffer.GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	// Clear Render Targets
	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };


	CD3DX12_CPU_DESCRIPTOR_HANDLE pd3dRTVCPUDescriptorHandle[] = {
		currentGBuffer.GBuffers[0].GetResource()->GetRTVHandle(),
		currentGBuffer.GBuffers[1].GetResource()->GetRTVHandle(),
		currentGBuffer.GBuffers[2].GetResource()->GetRTVHandle(),
	};

	for (int i = 0; i < GBuffer::g_unNumGBuffers; ++i) {
		pd3dCommandList->ClearRenderTargetView(pd3dRTVCPUDescriptorHandle[i], pfClearColor, 0, NULL);
	}

	//CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = RENDER->GetDepthStencilBufferHandle();

	const auto& dsvRef = RENDER->GetDepthStencilBuffer();
	auto pDSV = dsvRef.GetResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = pDSV->GetDSVHandle();
	pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);

	pd3dCommandList->OMSetRenderTargets(_countof(pd3dRTVCPUDescriptorHandle), pd3dRTVCPUDescriptorHandle, FALSE, &d3dDSVDescriptorHandle);
}

void GBufferPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	m_CachedData.Clear();
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	// Frustum Culling
	const BoundingFrustum& xmFrustumWorld = CUR_SCENE->GetCamera()->GetFrustumWorld();

	{
		const std::vector<std::shared_ptr<IGameObject>>& inputResource = RENDER->GetObjectsToRender();
		m_CachedData.pObjFrustumCulled.reserve(inputResource.size());

		std::copy_if(inputResource.begin(), inputResource.end(), std::back_inserter(m_CachedData.pObjFrustumCulled), [&xmFrustumWorld](const auto& pObj) {
			const auto pCollider = pObj->GetComponentFromRoot<ICollider>();
			return (pCollider) ? pCollider->IsInFrustum(xmFrustumWorld) : true;
		});
	}

	m_unFrustumCulled = m_CachedData.pObjFrustumCulled.size();

	// Gather draw unit
	BindGeometryData(pd3dCommandList, outDescHandle);

	if (CUR_SCENE->GetTerrain() != nullptr) {

		SpatialQueryDesc terrainQuerDesc{};
		terrainQuerDesc.unLayerMask = SPATIAL_TERRAIN;
		terrainQuerDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
		terrainQuerDesc.bIncludeStatic = true;
		terrainQuerDesc.bIncludeDynamic = false;

		SpatialQueryResult terrainRenderCandidates = CUR_SCENE->GetWorld().GetSpatial().QueryFrustum(xmFrustumWorld, terrainQuerDesc);
		const auto& terrainCulled = terrainRenderCandidates.pTerrainComponents;

		m_CachedData.pTerrainComponentFrustumCulled = terrainCulled;
		BindTerrainData(pd3dCommandList, outDescHandle);
	}

	// Set Render Target
	SetRenderTargets(pd3dCommandList);
}

void GBufferPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	for (const auto& pObj : m_CachedData.pObjFrustumCulled) {
		const auto& pMeshRenderer = pObj->GetComponent<MeshRenderer>();
		auto [idx, bInserted] = m_CachedData.frustumCulledMap.Insert(pMeshRenderer->GetID(), { pMeshRenderer.get(), std::vector<const IGameObject*>{ pObj.get() } });
		if (!bInserted) {
			m_CachedData.frustumCulledMap[idx].second.push_back(pObj.get());
		}
	}

	// materialData
	size_t newSize = m_CachedData.frustumCulledMap.Size();
	m_CachedData.materialMap.Reserve(newSize);
	m_CachedData.textureMap.Reserve(newSize * 4);

	m_RenderQueueCached.clear();
	size_t estimatedRenderQueueSize = 0;
	for (const auto& [k, v] : m_CachedData.frustumCulledMap.GetElements()) {
		estimatedRenderQueueSize += k->GetMeshes().size();
	}

	m_RenderQueueCached.reserve(estimatedRenderQueueSize);
	size_t estimatedWorldTransformSize = 0;
	for (const auto& [k, v] : m_CachedData.frustumCulledMap.GetElements()) {
		estimatedWorldTransformSize += v.size();
	}

	m_CachedData.sbWorldTransformDatas.reserve(estimatedWorldTransformSize);
	m_CachedData.sbBoneTransformDatas.reserve(estimatedRenderQueueSize * 10);
	m_CachedData.nBoneOffsets.reserve(estimatedWorldTransformSize);

	for (auto& [k, v] : m_CachedData.frustumCulledMap.GetElements()) {
		// Prepare
		const auto& pMeshes = k->GetMeshes();
		const auto& materialHandles = k->GetMaterialHandles();

		const uint32 unWorldTransformOffset = m_CachedData.sbWorldTransformDatas.size();
		for (const auto pObj : v) {
			const Matrix& mtxWorld = pObj->GetWorldMatrix();
			Matrix mtxInvWorld = mtxWorld.Invert();

			m_CachedData.sbWorldTransformDatas.emplace_back(
				mtxWorld.Transpose(),
				mtxInvWorld.Transpose()
			);
		}

		std::vector<int32> nBoneOffsets;
		nBoneOffsets.reserve(v.size());
		for (const auto pObj : v) {
			const AnimationController* pAnim = pObj->GetComponentFromRoot<AnimationController>().get();
			if (!pAnim) continue;

			auto [it, bInserted] = m_CachedData.animationInstancingData.emplace(pAnim, m_CachedData.sbBoneTransformDatas.size());
			if (bInserted) {
				nBoneOffsets.emplace_back(m_CachedData.sbBoneTransformDatas.size());
				const auto& boneTransforms = pAnim->GetFinalOutput();
				m_CachedData.sbBoneTransformDatas.insert(m_CachedData.sbBoneTransformDatas.end(), boneTransforms.begin(), boneTransforms.end());
			}
			else {
				nBoneOffsets.emplace_back(it->second.unOffset);
			}
		}

		const uint32 unBoneOffsetStart = m_CachedData.nBoneOffsets.size();
		const uint32 unBoneOffsetCount = nBoneOffsets.size();
		m_CachedData.nBoneOffsets.insert(m_CachedData.nBoneOffsets.end(), nBoneOffsets.begin(), nBoneOffsets.end());

		for (int32 meshIdx = 0; meshIdx < pMeshes.size(); ++meshIdx) {
			CB_INSTANCE_DATA instanceData{};

			auto [idx, bInserted] = m_CachedData.materialMap.Insert(materialHandles[meshIdx].GetID(), materialHandles[meshIdx].GetResource()->GetMaterialData());
			instanceData.gnMaterialIndex = idx;

			const auto& pMaterial = materialHandles[meshIdx].GetResource();
			auto& texRefs = pMaterial->GetTextureRefs();
			for (int32 texIdx = 0; texIdx < 4; ++texIdx) {
				if (!texRefs[texIdx].IsValid()) {
					instanceData.gnTextureIndex[texIdx] = -1;
					continue;
				}

				auto [idx, bInserted] = m_CachedData.textureMap.Insert(texRefs[texIdx].GetID(), &texRefs[texIdx]);
				instanceData.gnTextureIndex[texIdx] = idx;
			}

			// Set
			RenderParameter renderParameter;
			const auto& pPSOs = pMaterial->GetShader()->GetPipelineStates();
			renderParameter.pd3dPipelineState = (pMaterial->HasAlphaMask()) ? pPSOs[1] : pPSOs[0];
			renderParameter.cbInstanceData = instanceData;
			renderParameter.nInstances = v.size();
			renderParameter.cbInstanceData.gnWorldTransformOffset = unWorldTransformOffset;
			renderParameter.unBoneOffsetStart = unBoneOffsetStart;
			renderParameter.unBoneOffsetCount = unBoneOffsetCount;

			m_RenderQueueCached.emplace_back(pMeshes[meshIdx].get(), renderParameter);
		}
	}

	// Bind Per pass data
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;

	// rootParam[11]
	const auto& worldTransformDatas = m_CachedData.sbWorldTransformDatas;
	auto worldTransformSBuffer = RENDER->AllocSBuffer<WorldTransformData>(worldTransformDatas.size());
	worldTransformSBuffer.WriteData(worldTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle, worldTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// rootParam[12]
	const auto& boneTransformDatas = m_CachedData.sbBoneTransformDatas;
	auto boneTransformSBuffer = RENDER->AllocSBuffer<Matrix>(boneTransformDatas.size());
	boneTransformSBuffer.WriteData(boneTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), boneTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	const auto& boneOffsets = m_CachedData.nBoneOffsets;
	auto boneOffsetSBuffer = RENDER->AllocSBuffer<int32>(boneOffsets.size());
	boneOffsetSBuffer.WriteData(boneOffsets);
	m_CachedData.d3dBoneOffsetGPUAddress = boneOffsetSBuffer.GPUAddress;

	// rootParam[13]
	const auto& materialDatas = m_CachedData.materialMap.GetElements();
	auto materialSBuffer = RENDER->AllocSBuffer<MaterialData>(materialDatas.size());
	materialSBuffer.WriteData(materialDatas);

	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), materialSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Set
	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::PER_PASS_BUFFERS), outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1 + 1 + 1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1 + 1 + 1, unDescriptorInc);

	// Bind Texture : rootParam[14]
	const auto& texDatas = m_CachedData.textureMap.GetElements();
	const uint32 unNumTextures = texDatas.size();
	for (const auto& pTexHandle : texDatas) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE texCPUHandle = pTexHandle->GetResource()->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), texCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Set
	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::PER_PASS_TEXTURES), outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(unNumTextures, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(unNumTextures, unDescriptorInc);
}

void GBufferPass::BindTerrainData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	constexpr uint32 rootParamTerrainLayer = std::to_underlying(ROOT_PARAMETER::TERRAIN_LAYER);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	// cbTerrainLayerData
	CB_TERRAIN_LAYER_DATA cbData = pTerrain->MakeLayerCBData();
	ConstantBuffer cbTerrainLayer = RENDER->AllocCBuffer<CB_TERRAIN_LAYER_DATA>();
	cbTerrainLayer.WriteData(&cbData);

	CD3DX12_CPU_DESCRIPTOR_HANDLE terrainLayerHandle = outDescHandle.cpuHandle;
	DEVICE->CopyDescriptorsSimple(1, terrainLayerHandle, cbTerrainLayer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	terrainLayerHandle.Offset(1, unDescriptorInc);

	// gtxtTerrainAlbedo[4]
	// gtxtTerrainNormal[4]
	CD3DX12_CPU_DESCRIPTOR_HANDLE albedoHandleBase = terrainLayerHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalHandleBase = terrainLayerHandle;
	normalHandleBase.Offset(4, unDescriptorInc);

	const auto& terrainMaterialIDs = pTerrain->GetComponent<MeshRenderer>()->GetMaterialHandles();
	for (int i = 0; i < 4; ++i) {
		if (i < terrainMaterialIDs.size()) {
			if (terrainMaterialIDs[i].IsValid()) {
				auto& pMaterial = terrainMaterialIDs[i].GetResource();
				auto& pAlbedoTex = pMaterial->GetTexture(0);	// Albedo
				auto& pNormalTex = pMaterial->GetTexture(1);	// Normal

				DEVICE->CopyDescriptorsSimple(1, albedoHandleBase, pAlbedoTex->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				DEVICE->CopyDescriptorsSimple(1, normalHandleBase, pNormalTex->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}

		albedoHandleBase.Offset(1, unDescriptorInc);
		normalHandleBase.Offset(1, unDescriptorInc);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTerrainLayer, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(9, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(9, unDescriptorInc);
}

void GBufferPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	DrawGeometry(pd3dCommandList, outDescHandle);
	
	if (CUR_SCENE->GetTerrain() != nullptr) {
		DrawTerrain(pd3dCommandList, outDescHandle);
	}
}

void GBufferPass::DrawGeometry(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	constexpr uint32 rootParamInstanceData = std::to_underlying(ROOT_PARAMETER::PER_INSTANCE_DATA);
	constexpr uint32 rootParamBoneOffset = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM_OFFSETS);

	ID3D12PipelineState* pd3dLastPipelineState = nullptr;

	for (auto& [k, v] : m_RenderQueueCached) {
		ConstantBuffer instanceCBuffer = RENDER->AllocCBuffer<CB_INSTANCE_DATA>();
		instanceCBuffer.WriteData(&v.cbInstanceData);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamInstanceData, instanceCBuffer.GPUAddress);

		if (v.unBoneOffsetCount != 0) {
			D3D12_GPU_VIRTUAL_ADDRESS d3dBoneOffsetGPUAddress =
				m_CachedData.d3dBoneOffsetGPUAddress + v.unBoneOffsetStart * sizeof(int32);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamBoneOffset, d3dBoneOffsetGPUAddress);
		}

		auto pPSO = v.pd3dPipelineState;
		if (pd3dLastPipelineState != pPSO.Get()) {
			pd3dCommandList->SetPipelineState(pPSO.Get());
			pd3dLastPipelineState = pPSO.Get();
		}

		k->Render(pd3dCommandList, v.nInstances);
	}
}

void GBufferPass::DrawTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::TERRAIN_WORLD_TRANSFORM);
	constexpr uint32 rootParamTerrainComponent = std::to_underlying(ROOT_PARAMETER::TERRAIN_COMPONENT_AND_WEIGHTMAP);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	auto pd3dTerrainPipelineState = SHADER->Get<TerrainShader>()->GetPipelineStates()[0];
	pd3dCommandList->SetPipelineState(pd3dTerrainPipelineState.Get());

	Matrix mtxTerrainWorld = pTerrain->GetWorldMatrix().Transpose();
	auto worldTransformCBuffer = RENDER->AllocCBuffer<Matrix>();
	worldTransformCBuffer.WriteData(&mtxTerrainWorld);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamWorldTransform, worldTransformCBuffer.GPUAddress);

	for (const auto& pComponent : m_CachedData.pTerrainComponentFrustumCulled) {
		// Component
		CB_TERRAIN_COMPONENT_DATA cbData = pComponent->MakeCBData();
		ConstantBuffer cbTerrainComponents = RENDER->AllocCBuffer<CB_TERRAIN_COMPONENT_DATA>();
		cbTerrainComponents.WriteData(&cbData);

		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, cbTerrainComponents.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		// Texture
		auto weightMapHandle = pComponent->GetWeightMapRefs();
		CD3DX12_CPU_DESCRIPTOR_HANDLE weightMapCPUHandle = weightMapHandle.GetResource()->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, weightMapCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTerrainComponent, outDescHandle.gpuHandle);
		outDescHandle.gpuHandle.Offset(2, unDescriptorInc);

		const auto& terrainIndexRange = pComponent->GetIndexRange();
		pTerrainMesh->Render(pd3dCommandList, 1, terrainIndexRange.unStartIndex, terrainIndexRange.unIndexCount);
	}
}

void GBufferPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();
	const auto& currentGBuffer = RENDER->GetGBuffer();
	auto depthHandle = RENDER->GetDepthStencilBuffer();

	for (auto& gBuffer : currentGBuffer.GBuffers) {
		gBuffer.GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}
	depthHandle.GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE gBufferHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamGBuffer = std::to_underlying(ROOT_PARAMETER::G_BUFFER);

	for (const auto& gBuffer : currentGBuffer.GBuffers) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVHandle = gBuffer.GetResource()->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, gBufferHandle, rtSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gBufferHandle.Offset(1, unDescriptorInc);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthSRVHandle = depthHandle.GetResource()->GetSRVHandle();
	DEVICE->CopyDescriptorsSimple(1, gBufferHandle, depthSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gBufferHandle.Offset(1, unDescriptorInc);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamGBuffer, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(4, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(4, unDescriptorInc);
}

void GBufferPass::ShowDebugInfo()
{
	ImGui::Text("Render items in frustum: %d", m_CachedData.pObjFrustumCulled.size());
	ImGui::Text("Terrain components in frustum: %d", m_CachedData.pTerrainComponentFrustumCulled.size());
	
	if (ImGui::TreeNode("Objs to draw")) {
		for (const auto& param : m_RenderQueueCached) {
			std::string str = std::format(
				"IMesh {} - Instance : {}, Anim? : {}",
				(void*)param.first,
				param.second.nInstances,
				(param.second.unBoneOffsetCount != 0) ? "TRUE" : "FALSE");

			if (ImGui::TreeNode(str.c_str())) {
				ImGui::Indent(20.f);
				{
					ImGui::Text("cbInstanceData.Material Index : %d", param.second.cbInstanceData.gnMaterialIndex);
					ImGui::Text("cbInstanceData.Albedo Index : %d", param.second.cbInstanceData.gnTextureIndex[0]);
					ImGui::Text("cbInstanceData.Normal Index : %d", param.second.cbInstanceData.gnTextureIndex[1]);
					ImGui::Text("cbInstanceData.Metaillic Index : %d", param.second.cbInstanceData.gnTextureIndex[2]);
					ImGui::Text("cbInstanceData.Emissive Index : %d", param.second.cbInstanceData.gnTextureIndex[3]);

					//ImGui::Text("pAnimationControllers.size() : %d", param.second.pAnimationControllers.size());
					//ImGui::Text("sbWorldTransformData.size() : %d", param.second.sbWorldTransformData.size());
				}
				ImGui::Unindent(20.f);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Terrain to draw")) {
		int cnt = 0;
		for (const auto& pTerrainComp : m_CachedData.pTerrainComponentFrustumCulled) {
			std::string strTreeName = std::format("Terrain component #{}", cnt++);
			if (ImGui::TreeNode(strTreeName.c_str())) {
				Vector2 v2ComponentSize = pTerrainComp->GetComponentSize();
				auto indexRange = pTerrainComp->GetIndexRange();

				ImGui::Text("Component Size : %f, %f", v2ComponentSize.x, v2ComponentSize.y);
				ImGui::Text("Index begins : %d", indexRange.unStartIndex);
				ImGui::Text("Index count : %d", indexRange.unIndexCount);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

}
