#include "pch.h"
#include "ToneMappingPass.h"
#include "ToneMappingVolume.h"

#define TONE_MAPPING_USE_LUT

void ToneMappingPass::Initialize()
{
	CreatePipelineState();

	std::string strName = "ToneLUT";
	m_ToneMapLUT = TEXTURE->LoadUnorderedAccessTexture(strName, 1, g_unLUTSize, g_unLUTSize, g_unLUTSize, DXGI_FORMAT_R16G16B16A16_FLOAT);

	strName = "GradingLUT";
	m_GradingLUT = TEXTURE->LoadUnorderedAccessTexture(strName, 1, g_unLUTSize, g_unLUTSize, g_unLUTSize, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

void ToneMappingPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	constexpr auto rootParamToneMapping = std::to_underlying(ROOT_PARAMETER::TONE_MAPPING_DATA);
	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	const auto& toneMappingVolume = CUR_SCENE->GetToneMappingVolume();
	const auto& postProcessingVolume = CUR_SCENE->GetPostProcessingVolume();


#ifdef TONE_MAPPING_USE_LUT
	// TODO : Dirty LUT 를 갱신하고 Set
	// 1. 현재 Mode 가 Dirty 라면 갱신
	// 2. Look 인지 Dirty 라면 갱신
	// 3. LUT 를 SRV 로 Bind 하고 Draw
	if (toneMappingVolume.IsDirty(LUT_DIRTY_FLAG::TONE_MAPPER)) {
		// Resource barrier first
		const auto pUAV = static_pointer_cast<UnorderedAccessTexture>(m_ToneMapLUT.GetResource());
		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pd3dCommandList->SetPipelineState(m_pd3dLUTBakingPipelineState[0].Get());
		pd3dCommandList->SetComputeRootSignature(m_pd3dLUTRootSignature.Get());

		//cbuffer cbToneMappingData : register(b1, space0)
		CB_TONE_MAPPING_LUT_DATA data = toneMappingVolume.GetToneMapperLUTCBData();
		auto cBuffer = RENDER->AllocCBuffer<CB_TONE_MAPPING_LUT_DATA>();
		cBuffer.WriteData(&data);
		pd3dCommandList->SetComputeRootConstantBufferView(0, cBuffer.GPUAddress);

		//RWTexture3D<float4> gtxtLUTToBuild : register(u0, space0);
		auto bindHandle = outDescHandle.cpuHandle;
		DEVICE->CopyDescriptorsSimple(1, bindHandle, pUAV->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(1, outDescHandle.gpuHandle);
		outDescHandle.cpuHandle.Offset(1, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(1, nDescriptorInc);

		// LUTSize = 32 * 32 * 32
		// numthreads = 4, 4, 4
		// => Dispatch = 8, 8, 8
		pd3dCommandList->Dispatch(8, 8, 8);

		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		m_fLastToneLUTUpdated = TIME->GetTotalTime();
	}

	if (toneMappingVolume.IsDirty(LUT_DIRTY_FLAG::GRADING)) {
		// Resource barrier first
		const auto pUAV = static_pointer_cast<UnorderedAccessTexture>(m_GradingLUT.GetResource());
		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pd3dCommandList->SetPipelineState(m_pd3dLUTBakingPipelineState[1].Get());
		pd3dCommandList->SetComputeRootSignature(m_pd3dLUTRootSignature.Get());

		//cbuffer cbToneMappingData : register(b1, space0)
		CB_TONE_MAPPING_LUT_DATA data = toneMappingVolume.GetGradingLUTCBData();
		auto cBuffer = RENDER->AllocCBuffer<CB_TONE_MAPPING_LUT_DATA>();
		cBuffer.WriteData(&data);
		pd3dCommandList->SetComputeRootConstantBufferView(0, cBuffer.GPUAddress);

		//RWTexture3D<float4> gtxtLUTToBuild : register(u0, space0);
		auto bindHandle = outDescHandle.cpuHandle;
		DEVICE->CopyDescriptorsSimple(1, bindHandle, pUAV->GetUAVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		pd3dCommandList->SetComputeRootDescriptorTable(1, outDescHandle.gpuHandle);
		outDescHandle.cpuHandle.Offset(1, nDescriptorInc);
		outDescHandle.gpuHandle.Offset(1, nDescriptorInc);

		pd3dCommandList->Dispatch(8, 8, 8);

		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		m_fLastLookLUTUpdated = TIME->GetTotalTime();
	}
	
	// Set PS data
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dGlobalRootSignature.Get());

	CB_TONE_MAPPING_COMMON_DATA toneData = toneMappingVolume.GetCommonCBData();
	auto cBuffer1 = RENDER->AllocCBuffer<CB_TONE_MAPPING_COMMON_DATA>();
	cBuffer1.WriteData(&toneData);

	CB_SCREEN_FX_DATA fxData = postProcessingVolume.GetScreenFXCBData();
	auto cBuffer2 = RENDER->AllocCBuffer<CB_SCREEN_FX_DATA>();
	cBuffer2.WriteData(&fxData);

	const auto pToneLUTSRV = m_ToneMapLUT.GetResource();
	const auto pGradingLUTSRV = m_GradingLUT.GetResource();

	auto bindHandle = outDescHandle.cpuHandle;
	DEVICE->CopyDescriptorsSimple(1, bindHandle, cBuffer1.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, nDescriptorInc), cBuffer2.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//DEVICE->CopyDescriptorsSimple(2, bindHandle, cBuffer1.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, nDescriptorInc), pToneLUTSRV->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, nDescriptorInc), pGradingLUTSRV->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamToneMapping, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(4, nDescriptorInc);
	outDescHandle.gpuHandle.Offset(4, nDescriptorInc);

#else
	CB_TONE_MAPPING_DATA data = MakeCBData();
	auto paramCBuffer = RENDER->AllocCBuffer<CB_TONE_MAPPING_DATA>();
	paramCBuffer.WriteData(&data);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamToneMapping, paramCBuffer.GPUAddress);

#endif

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);
}

void ToneMappingPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(pd3dCommandList, 1);
}

void ToneMappingPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

void ToneMappingPass::ShowDebugInfo()
{
	ImGui::Text("Last ToneMap LUT Updated : %f", m_fLastToneLUTUpdated);
	ImGui::Text("Last Grading LUT Updated : %f", m_fLastLookLUTUpdated);
}

void ToneMappingPass::CreatePipelineState()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("QuadVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("ToneMappingPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;

		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}


	CreateRootSignature();
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineDesc{};
	{
		d3dComputePipelineDesc.pRootSignature = m_pd3dLUTRootSignature.Get();
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("ToneMapLUTCS");
		d3dComputePipelineDesc.CachedPSO = { nullptr, 0 };
		d3dComputePipelineDesc.NodeMask = 0;
		d3dComputePipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}
	
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dLUTBakingPipelineState[0].GetAddressOf()));

	{
		d3dComputePipelineDesc.CS = SHADER->GetShaderByteCode("GradingLUTCS");
	}
	
	hr = DEVICE->CreateComputePipelineState(&d3dComputePipelineDesc, IID_PPV_ARGS(m_pd3dLUTBakingPipelineState[1].GetAddressOf()));

}

void ToneMappingPass::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE1 d3dRootDescriptor[1];
	d3dRootDescriptor[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);

	CD3DX12_ROOT_PARAMETER1 d3dRootParameters[2];
	d3dRootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	d3dRootParameters[1].InitAsDescriptorTable(1, d3dRootDescriptor, D3D12_SHADER_VISIBILITY_ALL);


	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	d3dRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	d3dRootSignatureDesc.Desc_1_1.NumParameters = _countof(d3dRootParameters);
	d3dRootSignatureDesc.Desc_1_1.pParameters = d3dRootParameters;
	d3dRootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
	d3dRootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
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

	hr = DEVICE->CreateRootSignature(
		0,
		pd3dSignatureBlob->GetBufferPointer(),
		pd3dSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(m_pd3dLUTRootSignature.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}
}
