#include "pch.h"
#include "RenderManager.h"
#include "MeshRenderer.h"

ComPtr<ID3D12RootSignature> RenderManager::g_pd3dGlobalRootSignature = nullptr;

void RenderManager::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	CreateGlobalRootSignature(pd3dDevice, pd3dCommandList);
	CreateSkyboxPipelineState(pd3dDevice);

	m_pForwardPass = std::make_shared<ForwardPass>(pd3dDevice, pd3dCommandList);
}

void RenderManager::CreateGlobalRootSignature(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	m_pd3dDevice = pd3dDevice;

	// Global Root Signature
	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRanges[5];
	d3dDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, 0);	// Per Scene (Camera)
	d3dDescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);	// Per Scene (Cubemap)
	d3dDescriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, 0);	// Per Pass	 (World 변환)
	d3dDescriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 2, 0, 0);	// Per Object (Material, instance base) + 필요한경우 World
	d3dDescriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 2, 0, 0);	// Diffuse, Normal/Height

	CD3DX12_ROOT_PARAMETER d3dRootParameters[6];
	d3dRootParameters[0].InitAsDescriptorTable(1, &d3dDescriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);	// Scene (Camera)
	d3dRootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);						// Scene (Light)
	d3dRootParameters[2].InitAsDescriptorTable(1, &d3dDescriptorRanges[1], D3D12_SHADER_VISIBILITY_ALL);	// Scene (Cubemap(Skybox))
	d3dRootParameters[3].InitAsDescriptorTable(1, &d3dDescriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);	// Pass (World)
	d3dRootParameters[4].InitAsDescriptorTable(1, &d3dDescriptorRanges[3], D3D12_SHADER_VISIBILITY_ALL);	// Material
	d3dRootParameters[5].InitAsDescriptorTable(1, &d3dDescriptorRanges[4], D3D12_SHADER_VISIBILITY_ALL);	// Texture(Diffused, Normal/Height)

	// s0 : SkyboxSampler
	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc[2];
	d3dSamplerDesc[0].Init(0);
	d3dSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc[0].MinLOD = 0;
	d3dSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// s1 : TextureSampler (general)
	d3dSamplerDesc[1].Init(1);
	d3dSamplerDesc[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3dSamplerDesc[1].MinLOD = 0;
	d3dSamplerDesc[1].MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = _countof(d3dSamplerDesc);
		d3dRootSignatureDesc.pStaticSamplers = d3dSamplerDesc;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (pd3dErrorBlob) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(g_pd3dGlobalRootSignature.GetAddressOf()));

	// DescriptorHandle Heap For Draw
	D3D12_DESCRIPTOR_HEAP_DESC d3dHeapDesc;
	{
		d3dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dHeapDesc.NumDescriptors = DESCRIPTOR_PER_DRAW;
		d3dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		d3dHeapDesc.NodeMask = 0;
	}

	m_DescriptorHeapForDraw.Initialize(pd3dDevice, d3dHeapDesc);
}

void RenderManager::CreateSkyboxPipelineState(ComPtr<ID3D12Device> pd3dDevice)
{
	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = Shader::CompileShader(L"../HLSL/SkyboxShader.hlsl", "VSSkybox", "vs_5_1", m_pd3dVertexShaderBlob.GetAddressOf());
		d3dPipelineDesc.GS = Shader::CompileShader(L"../HLSL/SkyboxShader.hlsl", "GSSkybox", "gs_5_1", m_pd3dGeometryShaderBlob.GetAddressOf());
		d3dPipelineDesc.PS = Shader::CompileShader(L"../HLSL/SkyboxShader.hlsl", "PSSkybox", "ps_5_1", m_pd3dPixelShaderBlob.GetAddressOf());
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;	// GS가 점을 받음
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dSkyboxPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

void RenderManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	/*
			1. 기존 과제 방식대로 Map 과 vector를 병행
			2. vector로 Pre-Pass 수행
			3. Map에 보관된 인덱스를 따라 Forward, Differed 분할
			4. std::span view 를 이용하여 Forward, Differed 렌더링 각각 수행
			5. Post-Processing
			+ Sprite
	
	*/

	CUR_SCENE->GetCamera()->SetViewportsAndScissorRects(pd3dCommandList);

	pd3dCommandList->SetGraphicsRootSignature(g_pd3dGlobalRootSignature.Get());
	pd3dCommandList->SetDescriptorHeaps(1, m_DescriptorHeapForDraw.GetD3DDescriptorHeap().GetAddressOf());
	
	// Scene Data 바인딩
	DescriptorHandle descHandle = m_DescriptorHeapForDraw.GetDescriptorHandleFromHeapStart();
	
	CB_CAMERA_DATA camData = CUR_SCENE->GetCamera()->MakeCBData();
	CB_LIGHT_DATA lightData = CUR_SCENE->MakeLightData(); 
	ConstantBuffer& camCBuffer = RESOURCE->AllocCBuffer<CB_CAMERA_DATA>();
	ConstantBuffer& lightCBuffer = RESOURCE->AllocCBuffer<CB_LIGHT_DATA>();

	camCBuffer.WriteData(&camData);
	lightCBuffer.WriteData(&lightData);

	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, camCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	// 0
	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_SCENE_CAM_DATA, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	// 1
	pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_SCENE_LIGHT_DATA, lightCBuffer.GPUAddress);

	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	// Pass 수행
	// Run 안에서 descHandle 의 offset 이 사용된만큼 움직임
	m_pForwardPass->Run(m_pd3dDevice, pd3dCommandList, m_InstanceDatas[OBJECT_RENDER_FORWARD], descHandle);

	RenderSkybox(pd3dCommandList, descHandle);

}

void RenderManager::RenderSkybox(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descriptorHandleFromPassStart)
{
	std::shared_ptr<Texture> pSkyboxTexture = TEXTURE->Get("Skybox");
	if (!pSkyboxTexture) {
		return;
	}

	pd3dCommandList->SetPipelineState(m_pd3dSkyboxPipelineState.Get());
	DescriptorHandle descHandle = m_DescriptorHeapForDraw.GetDescriptorHandleFromHeapStart();
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, pSkyboxTexture->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_SCENE_SKYBOX, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);


	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dCommandList->DrawInstanced(1, 1, 0, 0);
}

void RenderManager::Clear()
{
	for (int i = 0; i < 2; ++i) {
		m_InstanceIndexMap[i].clear();
		m_InstanceDatas[i].clear();
		m_nInstanceIndex[i] = 0;
	}
}
