#include "pch.h"
#include "ForwardPass.h"

void ForwardPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const
{
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
		//CB_INSTANCE_DATA instanceData = fnBindCommonData(*k, materialIDForBind, textureIDForBind);

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

void ForwardPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 rootParamInstanceData = std::to_underlying(ROOT_PARAMETER::PER_INSTANCE_DATA);
	const uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	const uint32 rootParamBoneTransform = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM);

	auto pd3dAnimatedPipelineState = SHADER->Get<AnimatedShader>()->GetPipelineStates()[0];
	auto pd3StaticPipelineState = SHADER->Get<StandardShader>()->GetPipelineStates()[0];

	for (auto& [k, v] : m_RenderQueueCached) {
		/*pd3dCommandList->SetGraphicsRoot32BitConstants(
			rootParamInstanceData,
			unNum32BitsValues,
			&v.cbInstanceData,
			0
		);*/

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

void ForwardPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT DescriptorHandle& outDescHandle) const
{
	m_RenderQueueCached.clear();
}
