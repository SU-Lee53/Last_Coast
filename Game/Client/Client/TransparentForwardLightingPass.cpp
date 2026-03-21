#include "pch.h"
#include "TransparentForwardLightingPass.h"

void TransparentForwardLightingPass::Initialize()
{
	CreatePipelineState();
}

void TransparentForwardLightingPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
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
	auto pHDRRenderTarget = RENDER->GetCurrentHDRBuffer();
	auto pDSV = RENDER->GetDepthStencilBuffer();

	pHDRRenderTarget->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);			// 이전 DefferedLighting Pass 에서 ALL_SHADER_RESOURCE 로 바꾸었으므로 한번 전환이 필요함

	auto DSVHandle = pDSV->GetDSVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pHDRRenderTarget->GetRTVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &DSVHandle);
}

void TransparentForwardLightingPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	constexpr uint32 rootParamInstanceData = std::to_underlying(ROOT_PARAMETER::PER_INSTANCE_DATA);
	constexpr uint32 rootParamBoneTransform = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM);
	constexpr uint32 rootParamWorldIndex = std::to_underlying(ROOT_PARAMETER::WORLE_TRANSFORM_INDEX);

	for (auto& [k, v] : m_RenderQueueCached) {
		ConstantBuffer cbInstanceData = RENDER->AllocCBuffer<CB_INSTANCE_DATA>();
		cbInstanceData.WriteData(&v.cbInstanceData);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamInstanceData, cbInstanceData.GPUAddress);

		if (v.pAnimationController) {
			// Animated
			pd3dCommandList->SetPipelineState(m_pd3dAnimatedPipelineState.Get());

			// Set World Index
			pd3dCommandList->SetGraphicsRoot32BitConstant(rootParamWorldIndex, v.sbWorldTransformIndex, 0);

			const std::vector<Matrix>& mtxBoneTransforms = v.pAnimationController->GetFinalOutput();
			StructuredBuffer sbBoneTransforms = RENDER->AllocSBuffer<Matrix>(mtxBoneTransforms.size());
			sbBoneTransforms.WriteData(mtxBoneTransforms);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamBoneTransform, sbBoneTransforms.GPUAddress);

			k->Render(pd3dCommandList, 1);
		}
		else {
			// Static
			pd3dCommandList->SetPipelineState(m_pd3dStandardPipelineState.Get());

			// Set World Index
			pd3dCommandList->SetGraphicsRoot32BitConstant(rootParamWorldIndex, v.sbWorldTransformIndex, 0);
			k->Render(pd3dCommandList, 1);
		}
	}
}

void TransparentForwardLightingPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void TransparentForwardLightingPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, OUT DescriptorHandle& outDescHandle) const
{
	// materialData
	std::unordered_map<IMaterial::ID, size_t> materialIdxMap;
	materialIdxMap.reserve(frustumCulled.size());
	std::vector<IMaterial::ID> materialIDForBind;
	materialIDForBind.reserve(frustumCulled.size());

	std::unordered_map<Texture::ID, size_t> textureIdxMap;
	textureIdxMap.reserve(frustumCulled.size() * 4);
	std::vector<Texture::ID> textureIDForBind;
	textureIDForBind.reserve(frustumCulled.size() * 4);

	std::unordered_set<std::shared_ptr<IMesh>> pMeshes;

	std::vector<WorldTransformData> sbWorldTransformData;
	sbWorldTransformData.reserve(frustumCulled.size());

	uint32 unMaterialBindIndex = 0;
	uint32 unTextureBindIndex = 0;
	for (const auto& pObj : frustumCulled) {
		WorldTransformData worldTransformData{
			.mtxWorld = pObj->GetWorldMatrix().Transpose(),
			.mtxInvWorld = pObj->GetWorldMatrix().Invert().Transpose()
		};
		sbWorldTransformData.push_back(worldTransformData);

		const auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();

		for (auto [pMesh, materialID] : std::views::zip(pMeshRenderer->GetMeshes(), pMeshRenderer->GetMaterialIDs())) {
			RenderParameter renderParameter{};

			auto materialIt = materialIdxMap.find(materialID);
			if (materialIt == materialIdxMap.end()) {
				materialIdxMap.insert({ materialID, unMaterialBindIndex++ });
				materialIDForBind.push_back(materialID);

				auto pMaterial = MATERIAL->GetMaterialByID(materialID);
				for (auto texID : pMaterial->GetTextureIDs()) {
					if (texID == INVALID_ID) continue;
					auto texIt = textureIdxMap.find(texID);
					if (texIt == textureIdxMap.end()) {
						textureIdxMap.insert({ texID, unTextureBindIndex++ });
						textureIDForBind.push_back(texID);
					}
				}
			}

			renderParameter.cbInstanceData.gnMaterialIndex = materialIdxMap[materialID];

			auto pMaterial = MATERIAL->GetMaterialByID(materialID);
			const auto& texIDs = pMaterial->GetTextureIDs();
			for (uint32 i = 0; i < 4; ++i) {
				auto pTex = pMaterial->GetTexture(i);
				renderParameter.cbInstanceData.gnTextureIndex[i] = (pTex) ? textureIdxMap[texIDs[i]] : -1;
			}

			renderParameter.sbWorldTransformIndex = sbWorldTransformData.size() - 1;
			renderParameter.pAnimationController = pObj->GetComponentFromRoot<AnimationController>();

			m_RenderQueueCached.emplace_back(pMesh, renderParameter);
		}

	}

	// Bind
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	constexpr uint32 rootParamPerPass = std::to_underlying(ROOT_PARAMETER::PER_PASS_DATA);

	// Bind World Transforms
	auto worldSBuffer = RENDER->AllocSBuffer<WorldTransformData>(sbWorldTransformData.size());
	worldSBuffer.WriteData(sbWorldTransformData);
	pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, worldSBuffer.GPUAddress);

	// Bind Material
	std::vector<MaterialData> sbMaterialDatas;
	sbMaterialDatas.reserve(materialIDForBind.size());
	std::transform(materialIDForBind.begin(), materialIDForBind.end(), std::back_inserter(sbMaterialDatas), [](const Texture::ID id) {
		return MATERIAL->GetMaterialByID(id)->GetMaterialData();
	});

	auto materialSBuffer = RENDER->AllocSBuffer<MaterialData>(sbMaterialDatas.size());
	materialSBuffer.WriteData(sbMaterialDatas);

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
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamPerPass, outDescHandle.gpuHandle);
	outDescHandle.gpuHandle.Offset(1 + unNumTextures, unDescriptorInc);
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
