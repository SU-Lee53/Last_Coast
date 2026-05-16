#include "pch.h"
#include "TransparentForwardLightingPass.h"

void TransparentForwardLightingPass::Initialize()
{
	CreatePipelineState();
}

void TransparentForwardLightingPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	m_RenderQueueCached.clear();
	std::vector<TransparentForwardLightingPass::RenderParameter> renderParameters;

	const auto& pTransparentObjs = RENDER->GetTransparentObjectsToRender();

	if (pTransparentObjs.size() == 0) {
		return;
	}

	// Frustum culling
	std::vector<std::shared_ptr<IGameObject>> pFrustumCulled;
	pFrustumCulled.reserve(pTransparentObjs.size());
	const BoundingFrustum& xmFrustumWorld = CUR_SCENE->GetCamera()->GetFrustumWorld();
	std::copy_if(pTransparentObjs.begin(), pTransparentObjs.end(), std::back_inserter(pFrustumCulled), [&xmFrustumWorld](const auto& pObj) {
		const auto pCollider = pObj->GetComponentFromRoot<ICollider>();
		return (pCollider) ? pCollider->IsInFrustum(xmFrustumWorld) : true;
		});

	// Back to Front sort
	const auto v3CameraPos = CUR_SCENE->GetCamera()->GetPosition();
	std::sort(pFrustumCulled.begin(), pFrustumCulled.end(), [&v3CameraPos](const auto& lhs, const auto& rhs) {
		float fDistLhs = Vector3::DistanceSquared(v3CameraPos, lhs->GetTransform()->GetPosition());
		float fDistRhs = Vector3::DistanceSquared(v3CameraPos, rhs->GetTransform()->GetPosition());
		return fDistLhs > fDistRhs;
		});

	auto temp = m_RenderQueueCached.size();
	if (temp != 0) {
		__debugbreak();
	}
	BindGeometryData(pd3dCommandList, pFrustumCulled, outDescHandle);

	// Set Render Target
	auto pHDRRenderTarget = static_pointer_cast<RenderTargetTexture>(RENDER->GetHDRBuffer(1).GetResource());
	auto pDSV = static_pointer_cast<DepthStencilTexture>(RENDER->GetDepthStencilBuffer().GetResource());

	pHDRRenderTarget->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);			// 이전 DefferedLighting Pass 에서 ALL_SHADER_RESOURCE 로 바꾸었으므로 한번 전환이 필요함

	auto DSVHandle = pDSV->GetDSVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pHDRRenderTarget->GetRTVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &DSVHandle);
}

void TransparentForwardLightingPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	constexpr uint32 rootParamInstanceData = std::to_underlying(ROOT_PARAMETER::PER_INSTANCE_DATA);
	constexpr uint32 rootParamBoneOffset = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM_OFFSETS);

	for (auto& [k, v] : m_RenderQueueCached) {
		ConstantBuffer cbInstanceData = RENDER->AllocCBuffer<CB_INSTANCE_DATA>();
		cbInstanceData.WriteData(&v.cbInstanceData);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamInstanceData, cbInstanceData.GPUAddress);

		if (v.nBoneOffset != -1) {
			auto boneOffsetSBuffer = RENDER->AllocSBuffer<int32>(1);
			boneOffsetSBuffer.WriteData(v.nBoneOffset, 0);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamBoneOffset, boneOffsetSBuffer.GPUAddress);


			pd3dCommandList->SetPipelineState(m_pd3dAnimatedPipelineState.Get());
		}
		else {
			pd3dCommandList->SetPipelineState(m_pd3dStandardPipelineState.Get());
		}

		k->Render(pd3dCommandList, 1);
	}
}

void TransparentForwardLightingPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

void TransparentForwardLightingPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, OUT DescriptorHandle& outDescHandle) const
{
	m_CachedData.Clear();

	m_CachedData.materialMap.Reserve(frustumCulled.size());
	m_CachedData.textureMap.Reserve(frustumCulled.size());
	m_CachedData.sbWorldTransformDatas.reserve(frustumCulled.size());

	std::unordered_set<std::shared_ptr<IMesh>> pMeshes;

	uint32 unMaterialBindIndex = 0;
	uint32 unTextureBindIndex = 0;
	for (const auto& pObj : frustumCulled) {
		const auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();

		for (const auto& [pMesh, materialHandle] : std::views::zip(pMeshRenderer->GetMeshes(), pMeshRenderer->GetMaterialHandles())) {
			RenderParameter renderParameter{};

			const auto pMaterial = materialHandle.GetResource();
			auto [idx, bMaterialInserted] = m_CachedData.materialMap.Insert(materialHandle.GetID(), pMaterial->GetMaterialData());
			if (bMaterialInserted) {
				auto& texRefs = pMaterial->GetTextureRefs();
				for (uint32 i = 0; i < 4; ++i) {
					if (!texRefs[i].IsValid()) continue;
					auto [idx, bInserted] = m_CachedData.textureMap.Insert(texRefs[i].GetID(), &texRefs[i]);
				}
			}

			renderParameter.cbInstanceData.gnMaterialIndex = m_CachedData.materialMap.GetIndex(materialHandle.GetID());

			auto& texRefs = pMaterial->GetTextureRefs();
			for (uint32 i = 0; i < 4; ++i) {
				renderParameter.cbInstanceData.gnTextureIndex[i] = (texRefs[i].IsValid()) ? m_CachedData.textureMap.GetIndex(texRefs[i].GetID()) : -1;
			}

			renderParameter.cbInstanceData.gnWorldTransformOffset = m_CachedData.sbWorldTransformDatas.size();
			m_CachedData.sbWorldTransformDatas.emplace_back(
				pObj->GetWorldMatrix().Transpose(),
				pObj->GetWorldMatrix().Invert().Transpose()
			);

			const AnimationController* pAnim = pObj->GetComponentFromRoot<AnimationController>().get();

			if (pAnim) {
				auto [it, bBoneInserted] = m_CachedData.animationOffsetData.emplace(pAnim, m_CachedData.sbBoneTransformDatas.size());
				if (bBoneInserted) {
					renderParameter.nBoneOffset = m_CachedData.sbBoneTransformDatas.size();
					const auto& boneTransforms = pAnim->GetFinalOutput();
					m_CachedData.sbBoneTransformDatas.insert(m_CachedData.sbBoneTransformDatas.end(), boneTransforms.begin(), boneTransforms.end());
				}
				else {
					renderParameter.nBoneOffset = it->second.unOffset;
				}
			}

			m_RenderQueueCached.emplace_back(pMesh, renderParameter);
		}

	}

	// Bind Per pass data
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	//constexpr uint32 rootParamPerPass = std::to_underlying(ROOT_PARAMETER::PER_PASS_DATA);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;

	// rootParam[11]
	const auto& worldTransformDatas = m_CachedData.sbWorldTransformDatas;
	auto worldTransformSBuffer = RENDER->AllocSBuffer<WorldTransformData>(worldTransformDatas.size());
	worldTransformSBuffer.WriteData(worldTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle, worldTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// rootParam[11]
	const auto& boneTransformDatas = m_CachedData.sbBoneTransformDatas;
	auto boneTransformSBuffer = RENDER->AllocSBuffer<Matrix>(boneTransformDatas.size());
	boneTransformSBuffer.WriteData(boneTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), boneTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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

void TransparentForwardLightingPass::CreatePipelineState()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ForwardStandardVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("ForwardLightingPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// Depth Test 함
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;	// Depth 기록은 안함
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dStandardPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dAnimatedInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 5, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	inputLayoutDesc.NumElements = d3dAnimatedInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dAnimatedInputElements.data();

	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ForwardAnimatedVS");
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dAnimatedPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
