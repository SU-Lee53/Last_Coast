#include "pch.h"
#include "RenderManager.h"
#include "MeshRenderer.h"
#include "TerrainObject.h"
#include "Skybox.h"
#include "RenderGraph.h"

ComPtr<ID3D12RootSignature> RenderManager::g_pd3dGlobalRootSignature = nullptr;

void RenderManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
	CreateFence();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRenderTarget();
	CreateDepthStencil();

	CreateGlobalRootSignature(pd3dDevice);

	//m_pForwardPass = std::make_shared<ForwardPass>(pd3dDevice, pd3dCommandList);

	// DescriptorHandle Heap For Draw
	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.NumDescriptors = DESCRIPTOR_PER_DRAW;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		d3dHeapDesc.NodeMask = 0;
	}

	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		m_DescriptorHeapForDraw[i].Initialize(pd3dDevice, d3dHeapDesc);
		m_ConstantBufferPool[i].Initialize(1000);
		m_StructuredBufferPool[i].Initialize(1'000'000, 1000);
		m_GBuffers[i].Initialize(i);

		{
			auto& [srvID, rtvID] = TEXTURE->LoadRenderTargetTexture(
				"HDR" + std::to_string(i),
				WinCore::g_dwClientWidth,
				WinCore::g_dwClientHeight,
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				DXGI_FORMAT_R16G16B16A16_FLOAT);

			m_HDRRenderTargetIDs[i].first = srvID;
			m_HDRRenderTargetIDs[i].second = rtvID;
		}

		{
			auto& [srvID, rtvID] = TEXTURE->LoadRenderTargetTexture(
				"LDR" + std::to_string(i),
				WinCore::g_dwClientWidth,
				WinCore::g_dwClientHeight,
				m_dxgiRenderTargetFormat,
				m_dxgiRenderTargetFormat);

			m_LDRRenderTargetIDs[i].first = srvID;
			m_LDRRenderTargetIDs[i].second = rtvID;
		}

	}

	m_pQuadMesh = std::make_shared<QuadMesh>(-1, 1);
}

void RenderManager::CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice)
{
	CD3DX12_DESCRIPTOR_RANGE1 d3dDescriptorRanges[16]; 
	// space0 : Per Scene (Frame) 
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0); // cbSceneData 
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gLightData 

	// space0 : Cascade shadow maps
	d3dDescriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);					// cbCascadeShadowMatrix
	d3dDescriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtCascadeShadowMaps[NUM_CASCADES]
	
	// space0 : Shadow maps
	d3dDescriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0); // cbShadowMatrix
	d3dDescriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 5, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtShadows[4]

	// space0 : GBuffer[3] + Depth[1]
	d3dDescriptorRanges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 9, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
	d3dDescriptorRanges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	// space0 : HDR Result
	d3dDescriptorRanges[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 13, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

	// space1 : Per Pass 
	d3dDescriptorRanges[9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0); // gMaterialData 
	d3dDescriptorRanges[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 1, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTextures

	// space2 : cbTerrainLayerData
	d3dDescriptorRanges[11].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 2, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0); // cbTerrainLayerData
	d3dDescriptorRanges[12].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 2, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainAlbedo[4] 
	d3dDescriptorRanges[13].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 6, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainNormal[4]

	// space2 : cbTerrainComponentData
	d3dDescriptorRanges[14].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 2, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0); // cbTerrainComponentData 
	d3dDescriptorRanges[15].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 2, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // gtxtTerrainWeightMap

	CD3DX12_ROOT_PARAMETER1 d3dRootParameters[13];
	// Per Scene
	d3dRootParameters[0].InitAsDescriptorTable(2, &d3dDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);	// Per Draw
	d3dRootParameters[1].InitAsDescriptorTable(2, &d3dDescriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);	// Cascade Shadow maps
	d3dRootParameters[2].InitAsDescriptorTable(2, &d3dDescriptorRanges[4], D3D12_SHADER_VISIBILITY_ALL);	// Shadow maps
	d3dRootParameters[3].InitAsDescriptorTable(2, &d3dDescriptorRanges[6], D3D12_SHADER_VISIBILITY_ALL);	// G-Buffers
	d3dRootParameters[4].InitAsDescriptorTable(1, &d3dDescriptorRanges[8], D3D12_SHADER_VISIBILITY_ALL);	// HDR Result
	
	// Per Pass
	d3dRootParameters[5].InitAsDescriptorTable(2, &d3dDescriptorRanges[9], D3D12_SHADER_VISIBILITY_ALL);	// Per Pass

	// Per Instance(Draw)
	d3dRootParameters[6].InitAsConstantBufferView(0, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);	// cbInstanceData
	d3dRootParameters[7].InitAsConstantBufferView(4, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);	// cbLightCameraData
	d3dRootParameters[8].InitAsShaderResourceView(0, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);	// gWorldTransforms
	d3dRootParameters[9].InitAsShaderResourceView(1, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);	// gBoneTransforms
	d3dRootParameters[10].InitAsDescriptorTable(3, &d3dDescriptorRanges[11], D3D12_SHADER_VISIBILITY_ALL);	// TerrainLayer
	d3dRootParameters[11].InitAsDescriptorTable(2, &d3dDescriptorRanges[14], D3D12_SHADER_VISIBILITY_ALL);	// TerrainComponent
	d3dRootParameters[12].InitAsConstants(1, 3, 2, D3D12_SHADER_VISIBILITY_ALL);	// gnWorldTransformIndex

	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc[4];
	// s0 : SkyboxSampler
	d3dSamplerDesc[0].Init(0);
	d3dSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].MinLOD = 0;
	d3dSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s1 : WeightMap Sampler
	d3dSamplerDesc[1].Init(1);
	d3dSamplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	d3dSamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[1].MinLOD = 0;
	d3dSamplerDesc[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s2 : TextureSampler (General)
	d3dSamplerDesc[2].Init(2);
	d3dSamplerDesc[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[2].MinLOD = 0;
	d3dSamplerDesc[2].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s3 : ComparisonSampler
	d3dSamplerDesc[3].Init(3);
	d3dSamplerDesc[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	d3dSamplerDesc[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dSamplerDesc[3].MipLODBias = 0.0f;
	d3dSamplerDesc[3].MaxAnisotropy = 1;
	d3dSamplerDesc[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	d3dSamplerDesc[3].MinLOD = 0;
	d3dSamplerDesc[3].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[3].ShaderRegister = 3;
	d3dSamplerDesc[3].RegisterSpace = 0;
	d3dSamplerDesc[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	d3dRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	d3dRootSignatureDesc.Desc_1_1.NumParameters = _countof(d3dRootParameters);
	d3dRootSignatureDesc.Desc_1_1.pParameters = d3dRootParameters;
	d3dRootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(d3dSamplerDesc);
	d3dRootSignatureDesc.Desc_1_1.pStaticSamplers = d3dSamplerDesc;
	d3dRootSignatureDesc.Desc_1_1.Flags = d3dRootSignatureFlags;

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&d3dRootSignatureDesc, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(
		0, 
		pd3dSignatureBlob->GetBufferPointer(), 
		pd3dSignatureBlob->GetBufferSize(), 
		IID_PPV_ARGS(g_pd3dGlobalRootSignature.GetAddressOf())
	);
}

void RenderManager::BuildRenderGraph()
{
	m_RenderGraph.BuildGraph();
}

void RenderManager::Render()
{
	OnPrepareRender();

	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];

	pd3dCommandList->SetGraphicsRootSignature(g_pd3dGlobalRootSignature.Get());
	pd3dCommandList->SetDescriptorHeaps(1, m_DescriptorHeapForDraw[m_unCurrentContextIndex].GetD3DDescriptorHeap().GetAddressOf());
	
	auto pCamera = CUR_SCENE->GetCamera();
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);

	DescriptorHandle descHandle = m_DescriptorHeapForDraw[m_unCurrentContextIndex].GetDescriptorHandleFromHeapStart();
	BindPerSceneData(pd3dCommandList, descHandle);

	RenderPassInput input{};
	m_RenderGraph.Run(descHandle, pd3dCommandList, input);

	GUI->Render(pd3dCommandList);

	OnPostRender();
	Present();
}

void RenderManager::ShowDebugOptions()
{
	ImGui::Begin("RenderManager");

	ImGui::Text("Current Context : %d", m_unCurrentContextIndex);

	ImGui::SeparatorText("Render Queue");
	ImGui::Text("Opaque : %d", m_pObjectsToRender.size());
	ImGui::Text("Transparent: %d", m_pTransparentObjectsToRender.size());
	for (uint32 i = 0; i < g_unMaxSpriteLayers; ++i) {
		ImGui::Text("Sprite Layer[%d] : %d", i, m_pSpritesToRender[i].size());
	}

	ImGui::SeparatorText("Render Graph");
	m_RenderGraph.ShowDebugInfo();

	ImGui::SeparatorText("Frame Resources");
	{
		if (ImGui::TreeNode("Descriptor Heaps")) {
			ImGui::Text("DescriptorSize = %d", m_DescriptorHeapForDraw->m_uiDescriptorSize);
			ImGui::Text("CurrentDescriptorCount = %d", m_DescriptorHeapForDraw->m_uiCurrentDescriptorCount);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("ConstantBufferPool")) {
			m_ConstantBufferPool->ShowDebugInfo();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("StructuredBufferPool")) {
			m_StructuredBufferPool->ShowDebugInfo();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Render Targets")) {
			if (ImGui::TreeNode("Back Buffer")) {
				auto pBackBuffer = m_BackBufferIDs[0].second.GetResource();
				pBackBuffer->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("Main Depth Buffer")) {
				auto pDepthBuffer = m_DepthStencilID.second.GetResource();
				pDepthBuffer->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("G-Buffers")) {
				ImGui::Text("GBuffer[0][0] Info. Rest is same");
				auto pRTV = m_GBuffers[0].GBuffers[0].second.GetResource();
				pRTV->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("HDR")) {
				ImGui::Text("HDR[0][0] Info. Rest is same");
				auto pHDR = m_HDRRenderTargetIDs[0].second.GetResource();
				pHDR->ShowDebugInfo();

				ImGui::TreePop();
			}
			
			if (ImGui::TreeNode("LDR")) {
				ImGui::Text("HDR[0][0] Info. Rest is same");
				auto pLDR = m_LDRRenderTargetIDs[0].second.GetResource();
				pLDR->ShowDebugInfo();

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

	}


	ImGui::End();
}

void RenderManager::AddSprite(std::shared_ptr<Sprite> pSprite, RECT rect, uint32 unLayer)
{
	//m_pSpritesToRender[unLayer].emplace_back(pSprite, rect);
}

void RenderManager::Reset()
{
	m_pObjectsToRender.clear();
	m_pTransparentObjectsToRender.clear();

	for (uint32 i = 0; i < g_unMaxSpriteLayers; ++i) {
		m_pSpritesToRender[i].clear();
	}


	m_ConstantBufferPool[m_unCurrentContextIndex].Reset();
	m_StructuredBufferPool[m_unCurrentContextIndex].Reset();
}

void RenderManager::BindPerSceneData(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, OUT DescriptorHandle& outDescHandle)
{
	// descRange[0]  :  cbSceneData					1
	// descRange[1]  :  gLightData					1


	uint32 unDescIncrementSize = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	// Cache
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pSkybox = CUR_SCENE->GetSkybox();

	// descRange[0]
	CB_SCENE_DATA sceneCBData;
	{
		sceneCBData.gSceneGlobal.fTotalTime = TIME->GetTotalTime();
		sceneCBData.gSceneGlobal.fElapsedTime = TIME->GetTimeElapsed();
		sceneCBData.gSceneGlobal.nNumLights = CUR_SCENE->GetLightsInScene().size();
		sceneCBData.gSceneGlobal.v4GlobalAmbient = CUR_SCENE->GetGlobalAmbient();
		sceneCBData.gCamera = CUR_SCENE->GetCamera()->MakeCBData();
		sceneCBData.gSkybox = (pSkybox) ? CUR_SCENE->GetSkybox()->MakeCBData() : SkyboxData{};
		sceneCBData.nScreenSize = XMINT2{ 
			static_cast<int32>(WinCore::g_dwClientWidth), 
			static_cast<int32>(WinCore::g_dwClientHeight)
		};
	}
	auto perSceneCBuffer = AllocCBuffer<CB_SCENE_DATA>();
	perSceneCBuffer.WriteData(&sceneCBData);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, perSceneCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	outDescHandle.cpuHandle.Offset(1, unDescIncrementSize);

	// descRange[1]
	std::vector<LightData> lightDatas;
	lightDatas.reserve(sceneCBData.gSceneGlobal.nNumLights);
	for (const auto& pLight : CUR_SCENE->GetLightsInScene()) {
		lightDatas.push_back(pLight->MakeCBData());
	}
	auto lightSBuffer = AllocSBuffer<LightData>(lightDatas.size());
	lightSBuffer.WriteData(lightDatas);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, lightSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	outDescHandle.cpuHandle.Offset(1, unDescIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(std::to_underlying(ROOT_PARAMETER::PER_SCENE_DATA), outDescHandle.gpuHandle);
	outDescHandle.gpuHandle.Offset(2, unDescIncrementSize);	// 1 + 1

}

const TextureHandle RenderManager::GetCurrentBackBuffer() const
{
	return m_BackBufferIDs[m_unBackBufferIndex].second;
}

const TextureHandle RenderManager::GetDepthStencilBuffer() const
{
	return m_DepthStencilID.second;
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetCurrentBackBufferHandle() const
{
	return static_pointer_cast<RenderTargetTexture>(m_BackBufferIDs[m_unBackBufferIndex].second.GetResource())->GetRTVHandle();
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetDepthStencilBufferHandle() const
{
	return static_pointer_cast<DepthStencilTexture>(m_DepthStencilID.second.GetResource())->GetDSVHandle();
}

const GBuffer& RenderManager::GetCurrentGBuffer() const
{
	return m_GBuffers[m_unCurrentContextIndex];
}

const TextureHandle RenderManager::GetCurrentHDRBuffer() const
{
	return m_HDRRenderTargetIDs[m_unBackBufferIndex].second;
}

const TextureHandle RenderManager::GetCurrentLDRBuffer() const
{
	return m_LDRRenderTargetIDs[m_unBackBufferIndex].second;
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetCurrentHDRBufferHandle() const
{
	return static_pointer_cast<RenderTargetTexture>(m_HDRRenderTargetIDs[m_unBackBufferIndex].second.GetResource())->GetRTVHandle();
}

const CD3DX12_CPU_DESCRIPTOR_HANDLE RenderManager::GetCurrentLDRBufferHandle() const
{
	return static_pointer_cast<RenderTargetTexture>(m_LDRRenderTargetIDs[m_unBackBufferIndex].second.GetResource())->GetRTVHandle();
}

void RenderManager::OnPrepareRender()
{
	ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator = m_ppd3dCommandAllocator[m_unCurrentContextIndex];
	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];
	HRESULT hr{};

	hr = pd3dCommandAllocator->Reset();
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandAllocator");
		__debugbreak();
	}

	hr = pd3dCommandList->Reset(pd3dCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		SHOW_ERROR("Faied to reset CommandList");
		__debugbreak();
	}

	GetCurrentBackBuffer().GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	GetDepthStencilBuffer().GetResource()->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = GetCurrentBackBufferHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = GetDepthStencilBufferHandle();

	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };
	pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, NULL);
	pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);
	
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVDescriptorHandle);
}

void RenderManager::OnPostRender()
{
	ComPtr<ID3D12GraphicsCommandList> pd3dCommandList = m_ppd3dCommandList[m_unCurrentContextIndex];
	HRESULT hr;

	// Change rendered render target's resource state from D3D12_RESOURCE_STATE_RENDER_TARGET to D3D12_RESOURCE_STATE_PRESENT
	auto pBackBuffer = GetCurrentBackBuffer().GetResource();
	pBackBuffer->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_PRESENT);
	pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { pd3dCommandList.Get() };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
}

void RenderManager::CreateFence()
{
	m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

void RenderManager::CreateCommandQueueAndList()
{
	HRESULT hr{};

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc{};
	{
		d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	}

	hr = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, IID_PPV_ARGS(m_pd3dCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create CommandQueue");
	}

	// Create Command Allocator
	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_ppd3dCommandAllocator[i].GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to create CommandAllocator");
		}

		// Create Command List
		hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ppd3dCommandAllocator[i].Get(), NULL, IID_PPV_ARGS(m_ppd3dCommandList[i].GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to create CommandList");
		}
		// Close Command List(default is opened)
		hr = m_ppd3dCommandList[i]->Close();
	}

}

void RenderManager::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	dxgiSwapChainDesc.Width = WinCore::g_dwClientWidth;
	dxgiSwapChainDesc.Height = WinCore::g_dwClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = g_unMaxPendingFrames;
	dxgiSwapChainDesc.SampleDesc.Count = 1;
	dxgiSwapChainDesc.SampleDesc.Quality = 0;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc;
	fsSwapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain1> pSwapChain1;
	HRESULT hr = DXGI_FACTORY->CreateSwapChainForHwnd(
		m_pd3dCommandQueue.Get(),
		WinCore::g_hWnd,
		&dxgiSwapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		pSwapChain1.GetAddressOf());

	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain");
	}

	pSwapChain1->QueryInterface(IID_PPV_ARGS(m_pdxgiSwapChain.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("Failed to create SwapChain QueryInterface");
	}

	m_unBackBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	DXGI_FACTORY->MakeWindowAssociation(WinCore::g_hWnd, DXGI_MWA_NO_ALT_ENTER);
}

void RenderManager::CreateRenderTarget()
{
	HRESULT hr{};
	for (UINT i = 0; i < g_unMaxPendingFrames; ++i)
	{
		ComPtr<ID3D12Resource> pd3dRenderTarget;
		hr = m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(pd3dRenderTarget.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("Failed to get buffer from SwapChain");
		}

		m_BackBufferIDs[i] = TEXTURE->LoadRenderTargetTexture(pd3dRenderTarget, DXGI_FORMAT_UNKNOWN, m_dxgiRenderTargetFormat);
	}
}

void RenderManager::CreateDepthStencil()
{
	//m_DepthStencilID = TEXTURE->LoadDepthStencilTexture(
	//	"DSV",
	//	WinCore::g_dwClientWidth,
	//	WinCore::g_dwClientHeight,
	//	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	//	DXGI_FORMAT_D24_UNORM_S8_UINT
	//);

	m_DepthStencilID = TEXTURE->LoadDepthStencilTexture(
		"DSV",
		WinCore::g_dwClientWidth,
		WinCore::g_dwClientHeight,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_D32_FLOAT
	);
}

void RenderManager::Present()
{
	Fence();

	// 0 : V-Sync OFF
	// 1 : V-Sync ON
	uint32 unSyncInterval = D3DCore::g_unSyncInterval;
	uint32 unPresentFlags = 0;
	if (!unSyncInterval) {
		// if V-Sync is OFF
		unPresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr{};
	hr = m_pdxgiSwapChain->Present(unSyncInterval, unPresentFlags);

	if (hr == DXGI_ERROR_DEVICE_REMOVED) {
		__debugbreak();
	}

	m_unBackBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	// Prepare next frame
	uint32 unNextContextIndex = (m_unCurrentContextIndex + 1) % g_unMaxPendingFrames;
	WaitForFenceValue(m_un64LastFenceValues[unNextContextIndex]);

	m_unCurrentContextIndex = unNextContextIndex;
}

uint64 RenderManager::Fence()
{
	m_un64FenceValues++;
	m_pd3dCommandQueue->Signal(m_pd3dFence.Get(), m_un64FenceValues);
	m_un64LastFenceValues[m_unCurrentContextIndex] = m_un64FenceValues;
	return m_un64FenceValues;
}

void RenderManager::WaitForFenceValue(uint64 un64ExpectedFenceValue)
{
	if (m_pd3dFence->GetCompletedValue() < un64ExpectedFenceValue)
	{
		m_pd3dFence->SetEventOnCompletion(un64ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void RenderManager::WaitForGPUComplete()
{
	Fence();
	for (uint32 i = 0; i < g_unMaxPendingFrames; ++i) {
		WaitForFenceValue(m_un64LastFenceValues[i]);
	}
}

void RenderManager::ChangeSwapChainState()
{
	// TODO : ResourceTable 에서 리소스 삭제 구현 후 구현
}

void GBuffer::Initialize(uint32 nPendingFrameIndex)
{
	for (uint32 i = 0; i < g_unNumGBuffers; ++i) {
		GBuffers[i] = TEXTURE->LoadRenderTargetTexture(
			"GBuffer_" + std::to_string(i) + "_" + std::to_string(nPendingFrameIndex),
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}
