#include "pch.h"
#include "DirectionalCascadeShadowMapPass.h"
#include "TerrainObject.h"

void DirectionalCascadeShadowMapPass::Initialize()
{
	for (uint32 i = 0; i < RenderManager::g_unMaxPendingFrames; ++i) {
		for (uint32 j = 0; j < g_unNumCascade; ++j) {
			uint32 unShadowMapSize = g_unCascadeShadowMapSize[j];
			m_ShadowMapRef[i][j] = TEXTURE->LoadDepthStencilTexture(
				std::format("Cascade_{}_{}", i, j),
				unShadowMapSize,
				unShadowMapSize,
				DXGI_FORMAT_R32_FLOAT,
				DXGI_FORMAT_D32_FLOAT
			);
		}
	}

	CreatePipelineState();
}

void DirectionalCascadeShadowMapPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();

	std::vector<CD3DX12_RESOURCE_BARRIER> d3dResourceBarriers;
	d3dResourceBarriers.reserve(g_unNumCascade);
	for (int i = 0; i < g_unNumCascade; ++i) {
		auto pTex = m_ShadowMapRef[unCurrentContext][i].GetResource();
		pTex->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	ComputeCascade();
}

void DirectionalCascadeShadowMapPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	constexpr uint32 rootParamLightCameraData = std::to_underlying(ROOT_PARAMETER::LIGHT_CAMERA_DATA);

	BoundingBox xmAABBFitToWholeFrustum;
	std::array<Vector3, BoundingFrustum::CORNER_COUNT> v3Corners;
	CUR_SCENE->GetCamera()->GetFrustumWorld().GetCorners(v3Corners.data());
	BoundingBox::CreateFromPoints(xmAABBFitToWholeFrustum, BoundingFrustum::CORNER_COUNT, v3Corners.data(), sizeof(Vector3));

	std::vector<std::shared_ptr<IGameObject>> AABBCulled;
	{
		const std::vector<std::shared_ptr<IGameObject>>& inputResource = RENDER->GetObjectsToRender();
		AABBCulled.reserve(inputResource.size());

		std::copy_if(inputResource.begin(), inputResource.end(), std::back_inserter(AABBCulled), [&xmAABBFitToWholeFrustum](const auto& pObj) {
			const auto pCollider = pObj->GetComponentFromRoot<ICollider>();
			return (pCollider) ? pCollider->IsInAABB(xmAABBFitToWholeFrustum) : true;
		});
	}

	for (int i = 0; i < g_unNumCascade; ++i) {
		m_RenderQueueCached.clear();
		SetRenderTargets(pd3dCommandList, i);

		std::vector<std::shared_ptr<IGameObject>> frustumCulled;
		{
			const std::vector<std::shared_ptr<IGameObject>>& inputResource = RENDER->GetObjectsToRender();
			frustumCulled.reserve(AABBCulled.size());

			BoundingFrustum xmFrsutum = m_CascadeCached[i].xmFrustum;

			std::copy_if(AABBCulled.begin(), AABBCulled.end(), std::back_inserter(frustumCulled), [&xmFrsutum](const auto& pObj) {
				const auto pCollider = pObj->GetComponentFromRoot<ICollider>();
				return (pCollider) ? pCollider->IsInFrustum(xmFrsutum) : true;
			});
		}

		BindGeometryData(pd3dCommandList, frustumCulled, outDescHandle);

		auto lightCameraCBuffer = RENDER->AllocCBuffer<Matrix>();
		Matrix mtxViewProj = m_CascadeCached[i].mtxLightViewProj.Transpose();
		lightCameraCBuffer.WriteData(&mtxViewProj);
		pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamLightCameraData, lightCameraCBuffer.GPUAddress);

		DrawGeometry(pd3dCommandList, outDescHandle);

		if (CUR_SCENE->GetTerrain() != nullptr) {
			DrawTerrain(pd3dCommandList, outDescHandle);
		}
	}
}

void DirectionalCascadeShadowMapPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();

	std::vector<std::shared_ptr<Texture>> pDSVTextures;
	std::vector<CD3DX12_RESOURCE_BARRIER> d3dResourceBarriers;
	pDSVTextures.reserve(g_unNumCascade);
	d3dResourceBarriers.reserve(g_unNumCascade);
	for (int i = 0; i < g_unNumCascade; ++i) {
		auto pTex = m_ShadowMapRef[unCurrentContext][i].GetResource();
		pTex->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		pDSVTextures.push_back(pTex);
	}

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamCascadeShadowMap = std::to_underlying(ROOT_PARAMETER::CASCADE_SHADOW_MAPS);

	// Set toShadow matrix
	std::vector<Matrix> mtxToShadows;
	mtxToShadows.reserve(4);
	std::transform(m_CascadeCached.begin(), m_CascadeCached.end(), std::back_inserter(mtxToShadows), [](const CascadeCameraData& data) {return data.mtxToShadowMap.Transpose(); });
	
	auto toShadowCBuffer = RENDER->AllocCBuffer<CB_TO_SHADOW_MATRICES_DATA>();
	toShadowCBuffer.WriteData(mtxToShadows);
	DEVICE->CopyDescriptorsSimple(1, bindHandle, toShadowCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	bindHandle.Offset(1, unDescriptorInc);


	for (const auto& shadowMap : pDSVTextures) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsSRVHandle = shadowMap->GetSRVHandle();
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
	inputLayoutDesc.NumElements = d3dStandardInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dStandardInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ShadowStandardVS");
		d3dPipelineDesc.PS = { nullptr, 0 };
		d3dPipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = FALSE;
		d3dPipelineDesc.RasterizerState.DepthBias = 8000;
		d3dPipelineDesc.RasterizerState.DepthBiasClamp = 0.0f;
		d3dPipelineDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
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

	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dAnimatedInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	inputLayoutDesc.NumElements = d3dAnimatedInputElements.size();
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

	auto dsvRef = m_ShadowMapRef[unCurrentContext][unCascade];
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

void DirectionalCascadeShadowMapPass::BindGeometryData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::vector<std::shared_ptr<IGameObject>>& frustumCulled, OUT DescriptorHandle& outDescHandle) const
{
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

	// World Transforms
	m_RenderQueueCached.clear();
	size_t estimatedRenderQueueSize = 0;
	for (const auto& [k, v] : renderItems) {
		estimatedRenderQueueSize += k->GetMeshes().size();
	}
	m_RenderQueueCached.reserve(estimatedRenderQueueSize);
	for (auto& [k, v] : renderItems) {

		// Prepare
		const auto& pMeshes = k->GetMeshes();
		const auto& materialIDs = k->GetMaterialHandles();
		int32 nMeshes = pMeshes.size();

		for (int32 meshIdx = 0; meshIdx < k->GetMeshes().size(); ++meshIdx) {
			RenderParameter renderParameter;

			// Set
			for (const auto pObj : v) {
				Matrix mtxWorld = pObj->GetWorldMatrix();
				//Matrix mtxInvWorld = mtxWorld.Invert();

				WorldTransformData data{
					mtxWorld.Transpose(),
					Matrix::Identity,
				};

				renderParameter.sbWorldTransformData.push_back(data);
				if (auto pAnim = pObj->GetComponentFromRoot<AnimationController>().get()) {
					renderParameter.pAnimationControllers.push_back(pAnim);
				}
			}

			m_RenderQueueCached.push_back(std::make_pair(pMeshes[meshIdx].get(), renderParameter));
		}
	}

}

void DirectionalCascadeShadowMapPass::DrawGeometry(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	constexpr uint32 rootParamBoneTransform = std::to_underlying(ROOT_PARAMETER::BONE_TRANSFORM);

	for (auto& [k, v] : m_RenderQueueCached) {
		if (v.pAnimationControllers.size() != 0) {
			// Animated
			pd3dCommandList->SetPipelineState(m_pd3dAnimatedPipelineState.Get());

			for (size_t i = 0; i < v.pAnimationControllers.size(); ++i) {
				StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<WorldTransformData>(1);
				sbWorldTransforms.WriteData(v.sbWorldTransformData[i], 0);
				pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

				auto pAnimationCtrl = v.pAnimationControllers[i];
				const std::vector<Matrix>& mtxBoneTransforms = pAnimationCtrl->GetFinalOutput();
				StructuredBuffer sbBoneTransforms = RENDER->AllocSBuffer<Matrix>(mtxBoneTransforms.size());
				sbBoneTransforms.WriteData(mtxBoneTransforms);
				pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamBoneTransform, sbBoneTransforms.GPUAddress);

				k->RenderPosition(pd3dCommandList, 1);
			}
		}
		else {
			// Static
			pd3dCommandList->SetPipelineState(m_pd3dStandardPipelineState.Get());

			StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<WorldTransformData>(v.sbWorldTransformData.size());
			sbWorldTransforms.WriteData(v.sbWorldTransformData);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

			k->Render(pd3dCommandList, v.sbWorldTransformData.size());
		}
	}
}

void DirectionalCascadeShadowMapPass::DrawTerrain(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	constexpr uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	pd3dCommandList->SetPipelineState(m_pd3dStandardPipelineState.Get());

	StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<Matrix>(1);
	sbWorldTransforms.WriteData(pTerrain->GetWorldMatrix().Transpose(), 0);
	pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

	for (const auto& pComponent : pTerrainComponents) {
		const auto& terrainIndexRange = pComponent->GetIndexRange();
		pTerrainMesh->Render(pd3dCommandList, 1, terrainIndexRange.unStartIndex, terrainIndexRange.unIndexCount);
	}
}

void DirectionalCascadeShadowMapPass::ComputeCascade() const
{
	auto pCamera = CUR_SCENE->GetCamera();
	auto xmFrustum = pCamera->GetFrustumWorld();
	float fNearToFar = xmFrustum.Far - xmFrustum.Near;
	float fDistancePerCascade = fNearToFar / static_cast<float>(g_unNumCascade);

	float fFovY = pCamera->GetFovYInRadian();
	float fAspectRatio = pCamera->GetAspectRatio();

	for (uint32 i = 0; i < g_unNumCascade; ++i) {
		// 1. Generate cascade camera frustum
		BoundingFrustum xmCascadeFrustum;

		float fCascadeNear = xmFrustum.Near + (i * fDistancePerCascade);
		float fCascadeFar = xmFrustum.Near + ((i + 1) * fDistancePerCascade);
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
		const auto& pLights = CUR_SCENE->GetLightsInScene();	// Temporal
		std::shared_ptr<DirectionalLight> pLight = std::static_pointer_cast<DirectionalLight>(pLights[0]);	// Temporal

		Vector3 v3LightDir = pLight->m_v3Direction;
		v3LightDir.Normalize();
		Vector3 v3LightUpRef = (fabs(v3LightDir.Dot(Vector3::Up)) > 0.99f) ? Vector3::Backward : Vector3::Up;
		Vector3 v3LightRight = v3LightUpRef.Cross(v3LightDir);
		v3LightRight.Normalize();

		Vector3 v3LightUpReal = v3LightDir.Cross(v3LightRight);
		v3LightUpReal.Normalize();

		auto r = v3Corners | std::views::transform([&v3FrustumCenter](const Vector3& v3Point) {return XMVectorGetX(XMVector3LengthSq(v3Point - v3FrustumCenter)); });
		float fBackoff = *std::ranges::max_element(r);
		fBackoff = std::sqrt(fBackoff);

		Vector3 v3LightPos = v3FrustumCenter - (v3LightDir * fBackoff * ((g_unNumCascade - i) * 2.f));
		Matrix mtxLightView = XMMatrixLookToLH(v3LightPos, v3LightDir, v3LightUpReal);

		// 5. Calculate bounds using corners transformed into light space
		std::array<Vector3, nCorners> v3TransformedCorners;
		std::transform(v3Corners.begin(), v3Corners.end(), v3TransformedCorners.begin(), [&mtxLightView](const Vector3& v) { return XMVector3TransformCoord(v, mtxLightView); });
		auto [v3MinX, v3MaxX] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.x < rhs.x; });
		auto [v3MinY, v3MaxY] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.y < rhs.y; });
		auto [v3MinZ, v3MaxZ] = std::minmax_element(v3TransformedCorners.begin(), v3TransformedCorners.end(), [](const Vector3& lhs, const Vector3& rhs) { return lhs.z < rhs.z; });

		float fMinX = v3MinX->x, fMinY = v3MinY->y, fMinZ = v3MinZ->z;
		float fMaxX = v3MaxX->x, fMaxY = v3MaxY->y, fMaxZ = v3MaxZ->z;

		// 6. Add z-margin
		//float fMargin = 50.0f;
		//fMinZ -= fMargin;
		//fMaxZ += fMargin;

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
		
		Matrix mtxLightWorld = Matrix{
			v3LightRight.x, v3LightRight.y, v3LightRight.z, 0.f,
			v3LightUpReal.x, v3LightUpReal.y, v3LightUpReal.z, 0.f,
			v3LightDir.x, v3LightDir.y, v3LightDir.z, 0.f,
			v3LightPos.x, v3LightPos.y, v3LightPos.z, 1.f
		};

		// Test world
		//Matrix mtxInvView = mtxLightView.Invert();

		// 8. cache
		BoundingFrustum xmFrustumLight;
		BoundingFrustum::CreateFromMatrix(xmFrustumLight, mtxLightProj);
		xmFrustumLight.Transform(xmFrustumLight, mtxLightWorld);
		CascadeCameraData data{
			.xmFrustum = xmFrustumLight,
			.mtxLightViewProj = XMMatrixMultiply(mtxLightView, mtxLightProj),
			.mtxToShadowMap = XMMatrixMultiply(XMMatrixMultiply(mtxLightView, mtxLightProj), g_mtxToTexture)
		};

		m_CascadeCached[i] = data;
	}
}
