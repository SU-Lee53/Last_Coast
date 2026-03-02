#include "pch.h"
#include "GBufferPass.h"
#include "TerrainObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// GBufferPass

void GBufferPass::Initialize()
{
	for (uint32 i = 0; i < RenderManager::g_unMaxPendingFrames; ++i) {
		m_pRTVs[i].reserve(2);

		auto& [srvID0, rtvID0] = TEXTURE->LoadRenderTargetTexture(
			"GBuffer_0" + std::to_string(i),
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM);

		auto pRTV0 = std::static_pointer_cast<RenderTargetTexture>(TEXTURE->GetTextureByID(rtvID0, TEXTURE_RESOURCE_TYPE::RTV));
		m_pRTVs[i].push_back(pRTV0);

		auto& [srvID1, rtvID1] = TEXTURE->LoadRenderTargetTexture(
			"G_Buffer_1" + std::to_string(i),
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM);

		auto pRTV1 = std::static_pointer_cast<RenderTargetTexture>(TEXTURE->GetTextureByID(rtvID1, TEXTURE_RESOURCE_TYPE::RTV));
		m_pRTVs[i].push_back(pRTV1);

		auto& [srvID2, rtvID2] = TEXTURE->LoadRenderTargetTexture(
			"G_Buffer_2" + std::to_string(i),
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM);

		auto pRTV2 = std::static_pointer_cast<RenderTargetTexture>(TEXTURE->GetTextureByID(rtvID2, TEXTURE_RESOURCE_TYPE::RTV));
		m_pRTVs[i].push_back(pRTV2);

	}
}

void GBufferPass::SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const
{
	// Set Render Targets
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();

	CD3DX12_RESOURCE_BARRIER pd3dResourceBarriers[] = {
		m_pRTVs[unCurrentContext][0]->GetResourceBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, true),
		m_pRTVs[unCurrentContext][1]->GetResourceBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, true),
		m_pRTVs[unCurrentContext][2]->GetResourceBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, true)
	};

	pd3dCommandList->ResourceBarrier(_countof(pd3dResourceBarriers), pd3dResourceBarriers);

	// Clear Render Targets
	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };


	CD3DX12_CPU_DESCRIPTOR_HANDLE pd3dRTVCPUDescriptorHandle[] = {
		m_pRTVs[unCurrentContext][0]->GetRTVHandle(),
		m_pRTVs[unCurrentContext][1]->GetRTVHandle(),
		m_pRTVs[unCurrentContext][2]->GetRTVHandle()
	};

	for (int i = 0; i < m_pRTVs[unCurrentContext].size(); ++i) {
		pd3dCommandList->ClearRenderTargetView(pd3dRTVCPUDescriptorHandle[i], pfClearColor, 0, NULL);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = RENDER->GetDepthStencilBufferHandle();
	pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);

	pd3dCommandList->OMSetRenderTargets(_countof(pd3dRTVCPUDescriptorHandle), pd3dRTVCPUDescriptorHandle, FALSE, &d3dDSVDescriptorHandle);
}

void GBufferPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const
{
	// Set Render Target
	SetRenderTargets(pd3dCommandList);
	
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	// Frustum Culling
	std::vector<std::shared_ptr<IGameObject>> frustumCulled;
	{
		const std::vector<std::shared_ptr<IGameObject>>& inputResource = *(input.passResource.Get<std::vector<std::shared_ptr<IGameObject>>>());
		frustumCulled.reserve(inputResource.size());

		const BoundingFrustum& xmFrustumWorld = CUR_SCENE->GetCamera()->GetFrustumWorld();
		std::copy_if(inputResource.begin(), inputResource.end(), std::back_inserter(frustumCulled), [&xmFrustumWorld](const auto& pObj) {
			const auto pCollider = pObj->GetComponentFromRoot<ICollider>();
			return (pCollider) ? pCollider->IsInFrustum(xmFrustumWorld) : true;
			});
	}

	// Gather draw unit
	BindGeometryData(pd3dCommandList, frustumCulled, outDescHandle);

	if (CUR_SCENE->GetTerrain() != nullptr) {
		BindTerrainData(pd3dCommandList, outDescHandle);
	}
}

void GBufferPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	std::unordered_map<MeshRenderer::ID, size_t> renderItemIndex;
	renderItemIndex.reserve(frustumCulled.size());
	std::vector<std::pair<MeshRenderer*, std::vector<const IGameObject*>>> renderItems;
	renderItems.reserve(frustumCulled.size());
	for (const auto& pObj : frustumCulled) {
		const auto& pMeshRenderer = pObj->GetComponent<MeshRenderer>();
		auto it = renderItemIndex.find(pMeshRenderer->GetID());
		if (it == renderItemIndex.end()) {
			size_t idx = renderItems.size();
			renderItemIndex.emplace(pMeshRenderer->GetID(), idx);
			renderItems.emplace_back(pMeshRenderer.get(), std::vector<const IGameObject*>{pObj.get()});
		}
		else {
			size_t idx = it->second;
			renderItems[idx].second.push_back(pObj.get());
		}
	}

	// materialData
	std::unordered_map<IMaterial::ID, size_t> materialIdxMap;
	materialIdxMap.reserve(renderItems.size());
	std::vector<IMaterial::ID> materialIDForBind;
	materialIDForBind.reserve(renderItems.size());

	std::unordered_map<Texture::ID, size_t> textureIdxMap;
	textureIdxMap.reserve(renderItems.size() * 4);
	std::vector<Texture::ID> textureIDForBind;
	textureIDForBind.reserve(renderItems.size() * 4);

	m_RenderQueueCached.clear();
	size_t estimatedRenderQueueSize = 0;
	for (const auto& [k, v] : renderItems) {
		estimatedRenderQueueSize += k->GetMeshes().size();
	}
	m_RenderQueueCached.reserve(estimatedRenderQueueSize);
	for (auto& [k, v] : renderItems) {

		// Prepare
		const auto& pMeshes = k->GetMeshes();
		const auto& materialIDs = k->GetMaterialIDs();
		int32 nMeshes = pMeshes.size();

		for (int32 meshIdx = 0; meshIdx < k->GetMeshes().size(); ++meshIdx) {
			RenderParameter renderParameter;
			CB_INSTANCE_DATA instanceData{};

			auto matIt = materialIdxMap.find(materialIDs[meshIdx]);
			if (matIt == materialIdxMap.end()) {
				size_t idx = materialIDForBind.size();
				materialIdxMap.emplace(materialIDs[meshIdx], idx);
				materialIDForBind.push_back(materialIDs[meshIdx]);

				instanceData.gnMaterialIndex = idx;
			}
			else {
				instanceData.gnMaterialIndex = matIt->second;
			}

			const auto& pMaterial = MATERIAL->GetMaterialByID(materialIDs[meshIdx]);
			auto& texIDs = pMaterial->GetTextureIDs();
			for (int32 texIdx = 0; texIdx < 4; ++texIdx) {
				if (texIDs[texIdx] == INVALID_ID) {
					instanceData.gnTextureIndex[texIdx] = -1;
					continue;
				}

				auto texIt = textureIdxMap.find(texIDs[texIdx]);
				if (texIt == textureIdxMap.end()) {
					size_t idx = textureIDForBind.size();
					textureIdxMap.emplace(texIDs[texIdx], idx);
					textureIDForBind.push_back(texIDs[texIdx]);

					instanceData.gnTextureIndex[texIdx] = idx;
				}
				else {
					instanceData.gnTextureIndex[texIdx] = texIt->second;
				}
			}

			// Set
			renderParameter.cbInstanceData = instanceData;
			for (const auto pObj : v) {
				renderParameter.sbWorldTransformData.push_back(pObj->GetWorldMatrix().Transpose());
				if (auto pAnim = pObj->GetComponentFromRoot<AnimationController>().get()) {
					renderParameter.pAnimationControllers.push_back(pAnim);
				}
			}

			m_RenderQueueCached.push_back(std::make_pair(pMeshes[meshIdx].get(), renderParameter));
		}
	}

	// Bind Material
	std::vector<MaterialData> materialDatas;
	materialDatas.reserve(materialIDForBind.size());
	for (IMaterial::ID id : materialIDForBind) {
		materialDatas.push_back(MATERIAL->GetMaterialByID(id)->GetMaterialData());
	}

	auto materialSBuffer = RENDER->AllocSBuffer<MaterialData>(materialDatas.size());
	materialSBuffer.WriteData(materialDatas);

	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, materialSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

	// Bind Texture
	const uint32 unNumTextures = textureIDForBind.size();
	for (Texture::ID texID : textureIDForBind) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE texHandle = TEXTURE->GetTextureByID(texID, TEXTURE_RESOURCE_TYPE::SRV)->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, texHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	}

	// Set
	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::PER_PASS_DATA), outDescHandle.gpuHandle);
	outDescHandle.gpuHandle.Offset(1 + unNumTextures, unDescriptorInc);
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

	const auto& terrainMaterialIDs = pTerrain->GetComponent<MeshRenderer>()->GetMaterialIDs();
	for (int i = 0; i < 4; ++i) {
		if (i < terrainMaterialIDs.size()) {
			if (terrainMaterialIDs[i] != INVALID_ID) {
				auto& pMaterial = MATERIAL->GetMaterialByID(terrainMaterialIDs[i]);
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

void GBufferPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	DrawGeometry(pd3dCommandList, outDescHandle);
	
	if (CUR_SCENE->GetTerrain() != nullptr) {
		DrawTerrain(pd3dCommandList, outDescHandle);
	}
}

void GBufferPass::DrawGeometry(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	constexpr uint32 rootParamInstanceData = std::to_underlying(ROOT_PARAMETER::PER_INSTANCE_DATA);
	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	constexpr uint32 rootParamBoneTransform = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM);

	auto pd3dAnimatedPipelineState = SHADER->Get<AnimatedShader>()->GetPipelineStates()[0];
	auto pd3StaticPipelineState = SHADER->Get<StandardShader>()->GetPipelineStates()[0];

	for (auto& [k, v] : m_RenderQueueCached) {
		ConstantBuffer cbInstanceData = RENDER->AllocCBuffer<CB_INSTANCE_DATA>();
		cbInstanceData.WriteData(&v.cbInstanceData);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamInstanceData, cbInstanceData.GPUAddress);

		if (v.pAnimationControllers.size() != 0) {
			// Animated
			pd3dCommandList->SetPipelineState(pd3dAnimatedPipelineState.Get());

			for (size_t i = 0; i < v.pAnimationControllers.size(); ++i) {
				StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<Matrix>(1);
				sbWorldTransforms.WriteData(v.sbWorldTransformData[i], 0);
				pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

				auto pAnimationCtrl = v.pAnimationControllers[i];
				const std::vector<Matrix>& mtxBoneTransforms = pAnimationCtrl->GetFinalOutput();
				StructuredBuffer sbBoneTransforms = RENDER->AllocSBuffer<Matrix>(mtxBoneTransforms.size());
				sbBoneTransforms.WriteData(mtxBoneTransforms);
				pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamBoneTransform, sbBoneTransforms.GPUAddress);

				k->Render(pd3dCommandList, 1);
			}
		}
		else {
			// Static
			pd3dCommandList->SetPipelineState(pd3StaticPipelineState.Get());

			StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<Matrix>(v.sbWorldTransformData.size());
			sbWorldTransforms.WriteData(v.sbWorldTransformData);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

			k->Render(pd3dCommandList, v.sbWorldTransformData.size());
		}
	}
}

void GBufferPass::DrawTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	constexpr uint32 rootParamTerrainComponent = std::to_underlying(ROOT_PARAMETER::TERRAIN_COMPONENT_AND_WEIGHTMAP);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	auto pd3dTerrainPipelineState = SHADER->Get<TerrainShader>()->GetPipelineStates()[0];
	pd3dCommandList->SetPipelineState(pd3dTerrainPipelineState.Get());

	StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<Matrix>(1);
	sbWorldTransforms.WriteData(pTerrain->GetWorldMatrix().Transpose(), 0);
	pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

	for (const auto& pComponent : pTerrainComponents) {
		// Component
		CB_TERRAIN_COMPONENT_DATA cbData = pComponent->MakeCBData();
		ConstantBuffer cbTerrainComponents = RENDER->AllocCBuffer<CB_TERRAIN_COMPONENT_DATA>();
		cbTerrainComponents.WriteData(&cbData);

		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, cbTerrainComponents.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		// Texture
		Texture::ID weightMapID = pComponent->GetWeightMapID();
		CD3DX12_CPU_DESCRIPTOR_HANDLE weightMapHandle = TEXTURE->GetTextureByID(weightMapID, TEXTURE_RESOURCE_TYPE::SRV)->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, weightMapHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTerrainComponent, outDescHandle.gpuHandle);
		outDescHandle.gpuHandle.Offset(2, unDescriptorInc);

		const auto& terrainIndexRange = pComponent->GetIndexRange();
		pTerrainMesh->Render(pd3dCommandList, 1, terrainIndexRange.unStartIndex, terrainIndexRange.unIndexCount);
	}
}

void GBufferPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();

	auto pDepthBuffer = RENDER->GetDepthStencilBuffer();
	CD3DX12_RESOURCE_BARRIER d3dResourceBarriers[] = {
		m_pRTVs[unCurrentContext][0]->GetResourceBarrier(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, true),
		m_pRTVs[unCurrentContext][1]->GetResourceBarrier(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, true),
		m_pRTVs[unCurrentContext][2]->GetResourceBarrier(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, true),
		pDepthBuffer->GetResourceBarrier(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, true)
	};

	pd3dCommandList->ResourceBarrier(_countof(d3dResourceBarriers), d3dResourceBarriers);

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE gBufferHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamGBuffer = std::to_underlying(ROOT_PARAMETER::G_BUFFER);

	for (uint32 i = 0; i < m_pRTVs[unCurrentContext].size(); ++i) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVHandle = m_pRTVs[unCurrentContext][i]->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, gBufferHandle, rtSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gBufferHandle.Offset(1, unDescriptorInc);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthSRVHandle = pDepthBuffer->GetSRVHandle();
	DEVICE->CopyDescriptorsSimple(1, gBufferHandle, depthSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gBufferHandle.Offset(1, unDescriptorInc);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamGBuffer, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(3, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(3, unDescriptorInc);

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dBackBufferHandle = RENDER->GetCurrentBackBufferHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dBackBufferHandle, TRUE, nullptr);
}
