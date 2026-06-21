#include "pch.h"
#include "DirectionalCascadeShadowMapPass.h"
#include "TerrainObject.h"

void DirectionalCascadeShadowMapPass::Initialize()
{
	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		uint32 unShadowMapSize = g_unCascadeShadowMapSize[i];
		m_ShadowMapRefs[i] = TEXTURE->LoadDepthStencilTexture(
			std::format("Cascade_{}", i),
			unShadowMapSize,
			unShadowMapSize,
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_D32_FLOAT
		);
	}

	CreatePipelineState();
}

void DirectionalCascadeShadowMapPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const float fDeltaTime = DT;

	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		const float fUpdatePeriod = 1.0f / g_fCascadeUpdateFPS[i];

		m_fCascadeUpdateTimers[i] += fDeltaTime;

		if (m_fCascadeUpdateTimers[i] >= fUpdatePeriod) {
			m_bNeedUpdates[i] = true;

			m_fCascadeUpdateTimers[i] -= fUpdatePeriod;

			if (m_fCascadeUpdateTimers[i] >= fUpdatePeriod) {
				m_fCascadeUpdateTimers[i] = 0.0f;
			}
		}
		else {
			m_bNeedUpdates[i] = false;
		}
	}

	if (m_bFirstUpdate) {
		m_bNeedUpdates.fill(true);
		m_fCascadeUpdateTimers.fill(0.0f);
		m_bFirstUpdate = false;
	}

	for (int i = 0; i < g_unNumCascade; ++i) {
		if (!m_bNeedUpdates[i]) {
			continue;
		}

		auto pTex = m_ShadowMapRefs[i].GetResource();
		pTex->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	ComputeCascade();
}

void DirectionalCascadeShadowMapPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	constexpr uint32 rootParamLightCameraData = std::to_underlying(ROOT_PARAMETER::LIGHT_CAMERA_DATA);

	for (int i = 0; i < g_unNumCascade; ++i) {
		if (!m_bNeedUpdates[i]) {
			continue;
		}

		m_RenderQueueCached.clear();
		SetRenderTargets(pd3dCommandList, i);
		BoundingFrustum xmFrsutum = m_CascadeCached[i].xmCasterCullFrustum;

		m_FrustumCulledCached.clear();
		{
			//const std::vector<std::shared_ptr<IGameObject>>& inputResource = RENDER->GetObjectsToRender();
			m_FrustumCulledCached.reserve(300);

			SpatialQueryDesc objectShadowDesc{};
			objectShadowDesc.unLayerMask = SPATIAL_RENDERABLE | SPATIAL_CAST_SHADOW;
			objectShadowDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
			objectShadowDesc.bIncludeStatic = true;
			objectShadowDesc.bIncludeDynamic = true;

			SpatialQueryResult objectShadowCandidates = CUR_SCENE->GetWorld().GetSpatial().QueryFrustum(xmFrsutum, objectShadowDesc);
			for (const auto& pObj : objectShadowCandidates.pObjects) {
				pObj->AddToQueue(m_FrustumCulledCached);
			}

			// Add Player
			CUR_SCENE->GetPlayer()->AddToQueue(m_FrustumCulledCached);
		}

		BindGeometryData(pd3dCommandList, m_FrustumCulledCached, outDescHandle);

		auto lightCameraCBuffer = RENDER->AllocCBuffer<Matrix>();
		Matrix mtxViewProj = m_CascadeCached[i].mtxLightViewProj.Transpose();
		lightCameraCBuffer.WriteData(&mtxViewProj);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamLightCameraData, lightCameraCBuffer.GPUAddress);

		DrawGeometry(pd3dCommandList, outDescHandle);

		if (CUR_SCENE->GetTerrain() != nullptr) {
			SpatialQueryDesc terrainShadowDesc{};
			terrainShadowDesc.unLayerMask = SPATIAL_TERRAIN | SPATIAL_CAST_SHADOW;

			terrainShadowDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
			terrainShadowDesc.bIncludeStatic = true;
			terrainShadowDesc.bIncludeDynamic = false;

			SpatialQueryResult terrainShadowCandidates = CUR_SCENE->GetWorld().GetSpatial().QueryFrustum(xmFrsutum, terrainShadowDesc);
			const auto& terrainCulled = terrainShadowCandidates.pTerrainComponents;

			DrawTerrain(pd3dCommandList, terrainCulled, outDescHandle);
		}
	}
}

void DirectionalCascadeShadowMapPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();

	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		if (!m_bNeedUpdates[i]) {
			continue;
		}

		m_ShadowMapRefs[i].GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamCascadeShadowMap = std::to_underlying(ROOT_PARAMETER::CASCADE_SHADOW_MAPS);

	// Set toShadow matrix
	CB_TO_SHADOW_MATRICES_DATA toShadowData{};
	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		toShadowData.mtxToShadows[i] = m_CascadeCached[i].mtxToShadowMap.Transpose();
	}
	
	auto toShadowCBuffer = RENDER->AllocCBuffer<CB_TO_SHADOW_MATRICES_DATA>();
	toShadowCBuffer.WriteData(&toShadowData);
	DEVICE->CopyDescriptorsSimple(1, bindHandle, toShadowCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	bindHandle.Offset(1, unDescriptorInc);

	for (const auto& texRef : m_ShadowMapRefs) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsSRVHandle = texRef.GetResource()->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, bindHandle, dsSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		bindHandle.Offset(1, unDescriptorInc);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamCascadeShadowMap, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1 + g_unNumCascade, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1 + g_unNumCascade, unDescriptorInc);

	// Viewport & Scissor rect 복구
	auto pCamera = CUR_SCENE->GetCamera();
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
}

void DirectionalCascadeShadowMapPass::CreatePipelineState()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dStandardInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = static_cast<UINT>(d3dStandardInputElements.size());
	inputLayoutDesc.pInputElementDescs = d3dStandardInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ShadowStandardVS");
		d3dPipelineDesc.PS = { nullptr, 0 };
		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = FALSE;
		d3dPipelineDesc.RasterizerState.DepthBias = 5000;
		d3dPipelineDesc.RasterizerState.DepthBiasClamp = 0.05f;
		d3dPipelineDesc.RasterizerState.SlopeScaledDepthBias = 4.0f;
		d3dPipelineDesc.RasterizerState.DepthClipEnable = TRUE;
		d3dPipelineDesc.RasterizerState.MultisampleEnable = FALSE;
		d3dPipelineDesc.RasterizerState.AntialiasedLineEnable = FALSE;
		d3dPipelineDesc.RasterizerState.ForcedSampleCount = 0;
		d3dPipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 0;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dStandardPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ShadowTerrainVS");
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dTerrainPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dAnimatedInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	inputLayoutDesc.NumElements = static_cast<UINT>(d3dAnimatedInputElements.size());
	inputLayoutDesc.pInputElementDescs = d3dAnimatedInputElements.data();

	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ShadowAnimatedVS");
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dAnimatedPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

void DirectionalCascadeShadowMapPass::SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, uint32 unCascade) const
{
	auto unCurrentContext = RENDER->GetCurrentContextIndex();

	auto dsvRef = m_ShadowMapRefs[unCascade];
	auto pDSV = static_pointer_cast<DepthStencilTexture>(dsvRef.GetResource());

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVHandle = pDSV->GetDSVHandle();
	pd3dCommandList->ClearDepthStencilView(d3dDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
	pd3dCommandList->OMSetRenderTargets(0, nullptr, FALSE, &d3dDSVHandle);

	D3D12_VIEWPORT d3dViewport{
		.TopLeftX = 0.f,
		.TopLeftY = 0.f,
		.Width = static_cast<float>(g_unCascadeShadowMapSize[unCascade]),
		.Height = static_cast<float>(g_unCascadeShadowMapSize[unCascade]),
		.MinDepth = 0.f,
		.MaxDepth = 1.f,
	};

	D3D12_RECT d3dScissorRect{
		.left = 0,
		.top = 0,
		.right = (LONG)g_unCascadeShadowMapSize[unCascade],
		.bottom = (LONG)g_unCascadeShadowMapSize[unCascade],
	};

	pd3dCommandList->RSSetViewports(1, &d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect);

}

void DirectionalCascadeShadowMapPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<IGameObject*>& frustumCulled, OUT DescriptorHandle& outDescHandle)
{
	m_CachedData.Clear();

	m_CachedData.frustumCulledMap.Reserve(frustumCulled.size());

	for (const auto& pObj : frustumCulled) {
		const auto& pMeshRenderer = pObj->GetComponent<MeshRenderer>();
		auto [idx, bInserted] = m_CachedData.frustumCulledMap.Insert(pMeshRenderer->GetID(), { pMeshRenderer.get(), std::vector<const IGameObject*>{ pObj } });
		if (!bInserted) {
			m_CachedData.frustumCulledMap[idx].second.push_back(pObj);
		}
	}

	// World Transforms
	m_RenderQueueCached.clear();
	size_t estimatedRenderQueueSize = 0;
	for (const auto& [k, v] : m_CachedData.frustumCulledMap.GetElements()) {
		estimatedRenderQueueSize += k->GetMeshes().size();
	}
	m_RenderQueueCached.reserve(estimatedRenderQueueSize);

	m_CachedData.sbWorldTransformDatas.reserve(frustumCulled.size());
	m_CachedData.sbBoneTransformDatas.reserve(frustumCulled.size() * 10);
	m_CachedData.nBoneOffsets.reserve(frustumCulled.size());

	for (auto& [k, v] : m_CachedData.frustumCulledMap.GetElements()) {

		// Prepare
		const auto& pMeshes = k->GetMeshes();

		const uint32 unWorldTransformOffset = static_cast<uint32>(m_CachedData.sbWorldTransformDatas.size());
		for (const auto pObj : v) {
			const Matrix& mtxWorld = pObj->GetWorldMatrix();

			m_CachedData.sbWorldTransformDatas.emplace_back(
				mtxWorld.Transpose(),
				Matrix::Identity
			);
		}

		std::vector<int32> nBoneOffsets;
		nBoneOffsets.reserve(v.size());
		for (const auto pObj : v) {
			const auto pAnim = pObj->GetComponentFromRoot<AnimationController>().get();
			if (!pAnim) continue;

			auto [it, bInserted] = m_CachedData.animationInstancingData.emplace(pAnim, static_cast<int32>(m_CachedData.sbBoneTransformDatas.size()));
			if (bInserted) {
				nBoneOffsets.emplace_back(static_cast<int32>(m_CachedData.sbBoneTransformDatas.size()));
				const auto& boneTransforms = pAnim->GetFinalOutput();
				m_CachedData.sbBoneTransformDatas.insert(m_CachedData.sbBoneTransformDatas.end(), boneTransforms.begin(), boneTransforms.end());
			}
			else {
				nBoneOffsets.emplace_back(it->second.unOffset);
			}
		}

		const uint32 unBoneOffsetStart = static_cast<uint32>(m_CachedData.nBoneOffsets.size());
		const uint32 unBoneOffsetCount = static_cast<uint32>(nBoneOffsets.size());
		m_CachedData.nBoneOffsets.insert(m_CachedData.nBoneOffsets.end(), nBoneOffsets.begin(), nBoneOffsets.end());

		const int32 nMeshes = static_cast<int32>(pMeshes.size());
		for (int32 meshIdx = 0; meshIdx < nMeshes; ++meshIdx) {
			RenderParameter renderParameter;
			renderParameter.cbInstanceData.gnWorldTransformOffset = unWorldTransformOffset;
			renderParameter.nInstances = static_cast<int32>(v.size());
			renderParameter.unBoneOffsetStart = unBoneOffsetStart;
			renderParameter.unBoneOffsetCount = unBoneOffsetCount;
			renderParameter.pd3dPipelineState = (unBoneOffsetCount != 0) ? m_pd3dAnimatedPipelineState.Get() : m_pd3dStandardPipelineState.Get();

			m_RenderQueueCached.emplace_back(pMeshes[meshIdx].get(), renderParameter);
		}
	}

	// Bind Per pass data
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	constexpr uint32 rootParamPerPass = std::to_underlying(ROOT_PARAMETER::PER_PASS_BUFFERS);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;

	// rootParam[11]
	const auto& worldTransformDatas = m_CachedData.sbWorldTransformDatas;
	auto worldTransformSBuffer = RENDER->AllocSBuffer<WorldTransformData>(static_cast<uint32>(worldTransformDatas.size()));
	worldTransformSBuffer.WriteData(worldTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle, worldTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// rootParam[12]
	const auto& boneTransformDatas = m_CachedData.sbBoneTransformDatas;
	auto boneTransformSBuffer = RENDER->AllocSBuffer<Matrix>(static_cast<uint32>(boneTransformDatas.size()));
	boneTransformSBuffer.WriteData(boneTransformDatas);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), boneTransformSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	const auto& boneOffsets = m_CachedData.nBoneOffsets;
	auto boneOffsetSBuffer = RENDER->AllocSBuffer<int32>(static_cast<uint32>(boneOffsets.size()));
	boneOffsetSBuffer.WriteData(boneOffsets);
	m_CachedData.d3dBoneOffsetGPUAddress = boneOffsetSBuffer.GPUAddress;

	// Set
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamPerPass, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(3, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(3, unDescriptorInc);
}

void DirectionalCascadeShadowMapPass::DrawGeometry(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle)
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

		if (pd3dLastPipelineState != v.pd3dPipelineState) {
			pd3dCommandList->SetPipelineState(v.pd3dPipelineState);
			pd3dLastPipelineState = v.pd3dPipelineState;
		}

		k->RenderPosition(pd3dCommandList, v.nInstances);
	}
}

void DirectionalCascadeShadowMapPass::DrawTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<TerrainComponent*>& frustumCulled, OUT DescriptorHandle& outDescHandle)
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::TERRAIN_WORLD_TRANSFORM);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	pd3dCommandList->SetPipelineState(m_pd3dTerrainPipelineState.Get());

	Matrix mtxTerrainWorld = pTerrain->GetWorldMatrix().Transpose();
	auto worldTransformCBuffer = RENDER->AllocCBuffer<Matrix>();
	worldTransformCBuffer.WriteData(&mtxTerrainWorld);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamWorldTransform, worldTransformCBuffer.GPUAddress);

	for (const auto& pComponent : frustumCulled) {
		const auto& terrainIndexRange = pComponent->GetIndexRange();
		pTerrainMesh->RenderPosition(pd3dCommandList, 1, terrainIndexRange.unStartIndex, terrainIndexRange.unIndexCount);
	}
}

void DirectionalCascadeShadowMapPass::ComputeCascade()
{
	auto pCamera = CUR_SCENE->GetCamera();
	auto xmFrustum = pCamera->GetFrustumWorld();
	//float fNearToFar = xmFrustum.Far - xmFrustum.Near;
	//float fDistancePerCascade = fNearToFar / static_cast<float>(g_unNumCascade);
	float fCascadeNearBase = xmFrustum.Near;
	float fCascadeFarBase = std::min(xmFrustum.Far, g_fMaxShadowDistance);

	float fFovY = pCamera->GetFovYInRadian();
	float fAspectRatio = pCamera->GetAspectRatio();

	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		if (!m_bNeedUpdates[i]) {
			continue;
		}
		// 1. Generate cascade camera frustum
		BoundingFrustum xmCascadeFrustum;

		// uniform split
		//float fCascadeNear = xmFrustum.Near + (i * fDistancePerCascade);
		//float fCascadeFar = xmFrustum.Near + ((i + 1) * fDistancePerCascade);

		// Practical split
		float n = fCascadeNearBase, f = fCascadeFarBase;

		float p0 = static_cast<float>(i) / static_cast<float>(g_unNumCascade);
		float p1 = static_cast<float>(i + 1) / static_cast<float>(g_unNumCascade);

		float fLogNear = n * std::powf(f / n, p0);
		float fLogFar = n * std::powf(f / n, p1);

		float fUniformNear = n + (f - n) * p0;
		float fUniformFar = n + (f - n) * p1;

		float fCascadeNear = std::lerp(fUniformNear, fLogNear, g_fLambda);
		float fCascadeFar = std::lerp(fUniformFar, fLogFar, g_fLambda);

		Matrix mtxCascadeCameraProj = XMMatrixPerspectiveFovLH(fFovY, fAspectRatio, fCascadeNear, fCascadeFar);
		BoundingFrustum::CreateFromMatrix(xmCascadeFrustum, mtxCascadeCameraProj);
		xmCascadeFrustum.Transform(xmCascadeFrustum, pCamera->GetCameraWorldTransfromMatrix());

		// 2. Extract corners
		constexpr size_t nCorners = BoundingFrustum::CORNER_COUNT;
		std::array<Vector3, nCorners> v3Corners;
		xmCascadeFrustum.GetCorners(v3Corners.data());

		// 3. Calculate center
		Vector3 v3FrustumCenter = std::accumulate(v3Corners.begin(), v3Corners.end(), Vector3::Zero);
		v3FrustumCenter /= static_cast<float>(nCorners);

		// 4. Generate Light view matrix
		Vector3 v3LightDir = Vector3{ 1.f, -1.f, 1.f };
		if (const auto pLight = CUR_SCENE->GetSunLight()) {
			v3LightDir = pLight->m_v3Direction;
		}
		v3LightDir.Normalize();
		Vector3 v3LightUpRef = (fabs(v3LightDir.Dot(Vector3::Up)) > 0.99f) ? Vector3::Backward : Vector3::Up;
		Vector3 v3LightRight = v3LightUpRef.Cross(v3LightDir);
		v3LightRight.Normalize();

		Vector3 v3LightUpReal = v3LightDir.Cross(v3LightRight);
		v3LightUpReal.Normalize();

		auto r = v3Corners | std::views::transform([&v3FrustumCenter](const Vector3& v3Point) {return XMVectorGetX(XMVector3LengthSq(v3Point - v3FrustumCenter)); });
		float fBackoff = *std::ranges::max_element(r);
		fBackoff = std::sqrt(fBackoff);

		//Vector3 v3LightPos = v3FrustumCenter - (v3LightDir * fBackoff * ((g_unNumCascade - i) * 2.f));
		Vector3 v3LightPos = v3FrustumCenter - (v3LightDir * (fBackoff + g_fLightDistanceMargin));
		
		Matrix mtxLightView = XMMatrixLookToLH(v3LightPos, v3LightDir, v3LightUpReal);

		// 5. Calculate bounds using corners transformed into light space
		std::array<Vector3, nCorners> v3TransformedCorners;
		std::transform(v3Corners.begin(), v3Corners.end(), v3TransformedCorners.begin(), [&mtxLightView](const Vector3& v) { return XMVector3TransformCoord(v, mtxLightView); });
		auto [v3MinX, v3MaxX] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.x < rhs.x; });
		auto [v3MinY, v3MaxY] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.y < rhs.y; });
		auto [v3MinZ, v3MaxZ] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.z < rhs.z; });

		float fMinX = v3MinX->x, fMinY = v3MinY->y, fMinZ = v3MinZ->z;
		float fMaxX = v3MaxX->x, fMaxY = v3MaxY->y, fMaxZ = v3MaxZ->z;

		// 6. Add margin
		const float fShadowXYMargin = 5.0_m;
		fMinX -= fShadowXYMargin;
		fMaxX += fShadowXYMargin;
		fMinY -= fShadowXYMargin;
		fMaxY += fShadowXYMargin;
		fMinZ -= 80.0_m;
		fMaxZ += 20.0_m;

		// 7. off-center orthographic project
		const float fShadowMapSize = static_cast<float>(g_unCascadeShadowMapSize[i]);

		float fWidth = fMaxX - fMinX;
		float fHeight = fMaxY - fMinY;
		
		float fExtent = std::max(fWidth, fHeight);
		fWidth = fExtent;
		fHeight = fExtent;

		float fCenterX = (fMinX + fMaxX) * 0.5f;
		float fCenterY = (fMinY + fMaxY) * 0.5f;

		// Texel grid snapping
		float fUnitsPerTexelX = fWidth / fShadowMapSize;
		float fUnitsPerTexelY = fHeight / fShadowMapSize;

		fCenterX = std::floor(fCenterX / fUnitsPerTexelX) * fUnitsPerTexelX;
		fCenterY = std::floor(fCenterY / fUnitsPerTexelY) * fUnitsPerTexelY;

		// rebuild bounds
		fMinX = fCenterX - fWidth * 0.5f;
		fMaxX = fCenterX + fWidth * 0.5f;
		fMinY = fCenterY - fHeight * 0.5f;
		fMaxY = fCenterY + fHeight * 0.5f;

		Matrix mtxLightProj = XMMatrixOrthographicOffCenterLH(
			fMinX, fMaxX,
			fMinY, fMaxY,
			fMinZ, fMaxZ
		);
		
		Matrix mtxLightWorld = mtxLightView.Invert();

		// 8. cache
		BoundingFrustum xmShadowFrustum;
		BoundingFrustum::CreateFromMatrix(xmShadowFrustum, mtxLightProj);
		xmShadowFrustum.Transform(xmShadowFrustum, mtxLightWorld);

		float fCasterMinX = fMinX;
		float fCasterMaxX = fMaxX;
		float fCasterMinY = fMinY;
		float fCasterMaxY = fMaxY;

		constexpr float fCasterXYMargin = 50.0_m;
		fCasterMinX -= fCasterXYMargin;
		fCasterMaxX += fCasterXYMargin;
		fCasterMinY -= fCasterXYMargin;
		fCasterMaxY += fCasterXYMargin;
		Matrix mtxCasterCullProj = XMMatrixOrthographicOffCenterLH(
			fCasterMinX, fCasterMaxX,
			fCasterMinY, fCasterMaxY,
			fMinZ, fMaxZ
		);

		BoundingFrustum xmCasterCullFrustum;
		BoundingFrustum::CreateFromMatrix(xmCasterCullFrustum, mtxCasterCullProj);
		xmCasterCullFrustum.Transform(xmCasterCullFrustum, mtxLightWorld);

		CascadeCameraData data{
			.xmShadowFrustum = xmShadowFrustum,
			.xmCasterCullFrustum = xmCasterCullFrustum,
			.mtxLightViewProj = XMMatrixMultiply(mtxLightView, mtxLightProj),
			.mtxToShadowMap = XMMatrixMultiply(XMMatrixMultiply(mtxLightView, mtxLightProj), g_mtxToTexture)
		};

		m_CascadeCached[i] = data;
	}
}

void DirectionalCascadeShadowMapPass::ShowDebugInfo()
{
	/*if (ImGui::Button(std::format("Show Shadow Maps : {}", m_bShowShadowMaps ? "TRUE" : "FALSE").c_str())) {
		m_bShowShadowMaps = !m_bShowShadowMaps;
	}*/
}
