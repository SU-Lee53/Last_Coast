#include "pch.h"
#include "ToneMappingPass.h"

#define TONE_MAPPING_USE_LUT

void ToneMappingPass::Initialize()
{
	SetDefaultParameters(TONE_MAPPING_MODE::UNDEFINED, TONE_MAPPING_MODE::AGX);
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
	
#ifdef TONE_MAPPING_USE_LUT
	// TODO : Dirty LUT 를 갱신하고 Set
	// 1. 현재 Mode 가 Dirty 라면 갱신
	// 2. Look 인지 Dirty 라면 갱신
	// 3. LUT 를 SRV 로 Bind 하고 Draw
	auto nCurMode = std::to_underlying(m_eMode);

	if (true == m_bToneMapLUTDirtyFlags[nCurMode]) {
		// Resource barrier first
		const auto pUAV = static_pointer_cast<UnorderedAccessTexture>(m_ToneMapLUT.GetResource());
		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pd3dCommandList->SetPipelineState(m_pd3dLUTBakingPipelineState[0].Get());
		pd3dCommandList->SetComputeRootSignature(m_pd3dLUTRootSignature.Get());

		//cbuffer cbToneMappingData : register(b1, space0)
		CB_TONE_MAPPING_LUT_DATA data = MakeLUTCBData((DIRTY_UPDATE)nCurMode);
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

		m_bToneMapLUTDirtyFlags[nCurMode] = false;
		m_fLastToneLUTUpdated = TIME->GetTotalTime();
	}

	if (true == m_bGradingLUTDirtyFlag) {
		// Resource barrier first
		const auto pUAV = static_pointer_cast<UnorderedAccessTexture>(m_GradingLUT.GetResource());
		pUAV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pd3dCommandList->SetPipelineState(m_pd3dLUTBakingPipelineState[1].Get());
		pd3dCommandList->SetComputeRootSignature(m_pd3dLUTRootSignature.Get());

		//cbuffer cbToneMappingData : register(b1, space0)
		CB_TONE_MAPPING_LUT_DATA data = MakeLUTCBData(DIRTY_UPDATE::GRADING);
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

		m_bGradingLUTDirtyFlag = false;
		m_fLastLookLUTUpdated = TIME->GetTotalTime();
	}
	
	// Set PS data
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->SetGraphicsRootSignature(RenderManager::g_pd3dGlobalRootSignature.Get());

	CB_TONE_MAPPING_DATA data = MakeCBData();
	auto cBuffer = RENDER->AllocCBuffer<CB_TONE_MAPPING_DATA>();
	cBuffer.WriteData(&data);

	const auto pToneLUTSRV = m_ToneMapLUT.GetResource();
	const auto pGradingLUTSRV = m_GradingLUT.GetResource();

	auto bindHandle = outDescHandle.cpuHandle;
	DEVICE->CopyDescriptorsSimple(1, bindHandle, cBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, nDescriptorInc), pToneLUTSRV->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, nDescriptorInc), pGradingLUTSRV->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamToneMapping, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(3, nDescriptorInc);
	outDescHandle.gpuHandle.Offset(3, nDescriptorInc);

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
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ToneMappingVS");
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

CB_TONE_MAPPING_LUT_DATA ToneMappingPass::MakeLUTCBData(DIRTY_UPDATE eDirty) const
{
	CB_TONE_MAPPING_LUT_DATA data{};
	switch (eDirty)
	{
	case ToneMappingPass::DIRTY_UPDATE::AGX:
	{
		data.gToneMappingCommon0.x = (float)m_eMode;
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.AgX, sizeof(AgXParameters));
		break;
	}
	case ToneMappingPass::DIRTY_UPDATE::ACES:
	{
		data.gToneMappingCommon0.x = (float)m_eMode;
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.ACES, sizeof(ACESParameters));
		break;
	}
	case ToneMappingPass::DIRTY_UPDATE::UC2:
	{
		data.gToneMappingCommon0.x = (float)m_eMode;
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.UC2, sizeof(UC2Parameters));
		break;
	}
	case ToneMappingPass::DIRTY_UPDATE::GT:
	{
		data.gToneMappingCommon0.x = (float)m_eMode;
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.GT, sizeof(GTParameters));
		break;
	}
	case ToneMappingPass::DIRTY_UPDATE::GRADING:
	{
		::memcpy(&data, &m_Parameters.Grading, sizeof(GradingParameters));
		break;
	}
	default:
		std::unreachable();
		break;
	}

	return data;
}

CB_TONE_MAPPING_DATA ToneMappingPass::MakeCBData() const
{
	return CB_TONE_MAPPING_DATA{
		.fExposure = m_Parameters.Common.fExposure,
		.fGamma = m_Parameters.Common.fGamma,
		.fSaturation = m_Parameters.Common.fPostSaturation,
		.fInputScale = m_Parameters.Common.fInputScale,
		.fOutputScale = m_Parameters.Common.fOutputScale,
		.fGradingStrength = m_Parameters.Common.fGradingStrength,
		.pad = Vector2(0.f),
	};
}

void ToneMappingPass::SetDefaultParameters(TONE_MAPPING_MODE eModeBefore, TONE_MAPPING_MODE eModeAfter)
{
	if (eModeBefore == eModeAfter) {
		return;
	}

	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		m_Parameters.AgX = ToneMappingParameter::g_DefaultAgXParameters;
		break;
	}
	case TONE_MAPPING_MODE::GT:
	{
		m_Parameters.GT = ToneMappingParameter::g_DefaultGTParameters;
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		m_Parameters.UC2 = ToneMappingParameter::g_DefaultUC2Parameters;
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		m_Parameters.ACES = ToneMappingParameter::g_DefaultACESParameters;
		break;
	}
	default:
		std::unreachable();
		break;
	}

	m_Parameters.Grading = ToneMappingParameter::g_DefaultGradingParameters;

}

void ToneMappingPass::ShowDebugInfo()
{
	TONE_MAPPING_MODE eBefore = m_eMode;
	bool bModeChanged = ImGui::SliderInt("Mode", reinterpret_cast<int*>(&m_eMode), 0, std::to_underlying(TONE_MAPPING_MODE::COUNT) - 1, g_cstrModeName[std::to_underlying(m_eMode)]);

	if (bModeChanged) {
		SetDefaultParameters(eBefore, m_eMode);

		uint32 unBeforeMode = std::to_underlying(eBefore);
		uint32 unAfterMode = std::to_underlying(m_eMode);

		m_bToneMapLUTDirtyFlags[unBeforeMode] = true;
		m_bToneMapLUTDirtyFlags[unAfterMode] = true;
		m_bGradingLUTDirtyFlag = true;
	}

	const uint32 unMode = std::to_underlying(m_eMode);

	if (ImGui::Button("Reset Parameters")) {
		SetDefaultParameters(TONE_MAPPING_MODE::UNDEFINED, m_eMode);
		m_bToneMapLUTDirtyFlags[unMode] = true;
		m_bGradingLUTDirtyFlag = true;
	}

	ImGui::InputText("Save/Load name", &m_strSaveName);

	if (ImGui::Button(std::format("Save {} Parameters", g_cstrModeName[unMode]).c_str())) {
		SaveParametersToJson();
	}

	ImGui::SameLine();
	if (ImGui::Button(std::format("Load {} Parameters", g_cstrModeName[unMode]).c_str())) {
		LoadParametersFromJson();
	}

	if (ImGui::Button(std::format("Save Grading Parameters", g_cstrModeName[unMode]).c_str())) {
		SaveGrading();
	}

	ImGui::SameLine();
	if (ImGui::Button(std::format("Load Grading Parameters", g_cstrModeName[unMode]).c_str())) {
		LoadGrading();
	}

	ImGui::Text("Last ToneMap LUT Updated : %f", m_fLastToneLUTUpdated);
	ImGui::Text("Last Grading LUT Updated : %f", m_fLastLookLUTUpdated);

	int cnt{};

	ImGui::SeparatorText("Global / Outputs");

	ShowDragFloat(cnt++, "fExposure", reinterpret_cast<float*>(&m_Parameters.Common.fExposure), 0.01f, 0.f, 4.f, true, 0.f, 2.f, 1.f);
	ShowDragFloat(cnt++, "fGamma", reinterpret_cast<float*>(&m_Parameters.Common.fGamma), 0.01f, 1.0f, 3.0f, true, 2.0f, 2.4f, 2.2f);

	ShowDragFloat(cnt++, "fSaturation", reinterpret_cast<float*>(&m_Parameters.Common.fPostSaturation), 0.01f, 0.f, 2.f, true, 0.75f, 1.15, 1.f);
	ShowDragFloat(cnt++, "fInputScale", reinterpret_cast<float*>(&m_Parameters.Common.fInputScale), 0.01f, 0.25f, 4.f, true, 0.5f, 2.0f, 1.f);
	ShowDragFloat(cnt++, "fOutputScale", reinterpret_cast<float*>(&m_Parameters.Common.fOutputScale), 0.01f, 0.5f, 2.f, true, 0.8, 1.2, 1.f);
	ShowDragFloat(cnt++, "fGradingStrength", reinterpret_cast<float*>(&m_Parameters.Common.fGradingStrength), 0.01f, 0.0f, 1.0f, true, 0.0f, 1.0f, 1.0f);

	bool bToneLUTDirty = false;

	ImGui::SeparatorText("Tone mapper core parameters");

	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXWhite", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXWhite),		0.001f, 0.8f, 1.2f, true, 0.9f, 1.05f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXBlack", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXBlack),		0.001f, 0.0f, 0.15f, true, 0.0f, 0.05f, 0.f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXContrast", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXContrast),	0.0001f, 0.6f, 1.4f, true, 0.85f, 1.15f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXMinEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMinEV),		0.001f, -16.0f, -8.0f, true, -13.5f, -10.f, -12.47393f, "%.8f");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXMaxEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMaxEV),		0.001f, 2.0f, 8.0f, true, 3.f, 5.f, 4.026069f, "%.8f");

		if (m_Parameters.AgX.fAgXMaxEV < m_Parameters.AgX.fAgXMinEV) {
			m_Parameters.AgX.fAgXMaxEV = std::max(m_Parameters.AgX.fAgXMaxEV, m_Parameters.AgX.fAgXMinEV + 1.0f);
		}

		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		bToneLUTDirty |= ShowDragFloat(cnt++, "fACESExposureBias", reinterpret_cast<float*>(&m_Parameters.ACES.fACESExposureBias), 0.01f, 0.f, 4.f, true, 0.8f, 1.5f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fACESPreSaturation", reinterpret_cast<float*>(&m_Parameters.ACES.fACESPreSaturation), 0.01f, 0.f, 2.f, true, 0.9f, 1.1f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fACESPostSaturation", reinterpret_cast<float*>(&m_Parameters.ACES.fACESPostSaturation), 0.01f, 0.f, 2.f, true, 0.9f, 1.2f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fACESHighlightDesaturation", reinterpret_cast<float*>(&m_Parameters.ACES.fACESHighlightDesaturation), 0.01f, 0.f, 1.f, true, 0.0f, 0.35f, 0.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fACESCoreOutputScale", reinterpret_cast<float*>(&m_Parameters.ACES.fACESCoreOutputScale), 0.01f, 0.f, 2.f, true, 0.9f, 1.1f, 1.0f);
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		ImGui::SeparatorText("Shoulder params");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2A", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2A), 0.001f, 0.05f, 0.30, true, 0.05f, 0.30f, 0.15f);

		ImGui::SeparatorText("Linear section params");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2B", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2B), 0.001f, 0.20f, 0.80, true, 0.20f, 0.80f, 0.50f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2C", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2C), 0.001f, 0.05f, 0.30, true, 0.05f, 0.30f, 0.10f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2D", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2D), 0.001f, 0.05f, 0.40, true, 0.05f, 0.40f, 0.20f);

		ImGui::SeparatorText("Toe params");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2E", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2E), 0.001f, 0.00f, 0.08, true, 0.00f, 0.08f, 0.02f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2F", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2F), 0.001f, 0.01f, 0.50, true, 0.01f, 0.50f, 0.30f);

		ImGui::SeparatorText("Normalization");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2WhitePoint", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2WhitePoint), 0.1f, 1.0f, 20.0f, true, 4.0f, 16.0f, 11.2f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fUC2ExposureBias", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2ExposureBias), 0.1f, 0.1f, 4.0f, true, 0.5f, 3.0f, 0.15f);

		break;
	}
	case TONE_MAPPING_MODE::GT:
	{
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTMaxBrightness", reinterpret_cast<float*>(&m_Parameters.GT.fGTMaxBrightness),	0.001f, 0.8f, 2.0f, true, 0.9f, 1.3f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTContrast", reinterpret_cast<float*>(&m_Parameters.GT.fGTContrast),				0.001f, 0.5f, 2.0f, true, 0.85f, 1.25f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTLinearStart", reinterpret_cast<float*>(&m_Parameters.GT.fGTLinearStart),		0.001f, 0.0f, 0.5f, true, 0.12f, 0.3f, 0.22f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTLinearLength", reinterpret_cast<float*>(&m_Parameters.GT.fGTLinearLength),		0.0001f, 0.05f, 0.8f, true, 0.2f, 0.55f, 0.4f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTBlack", reinterpret_cast<float*>(&m_Parameters.GT.fGTBlack),					0.001f, 0.5f, 2.5f, true, 1.f, 1.6f, 1.33f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTPedestal", reinterpret_cast<float*>(&m_Parameters.GT.fGTPedestal),				0.00001f, 0.0f, 0.1f, true, 0.0f, 0.03f, 0.0f);
		
		m_Parameters.GT.fGTLinearStart = std::clamp(m_Parameters.GT.fGTLinearStart, 0.0f, 1.0f);

		m_Parameters.GT.fGTLinearLength = std::clamp(m_Parameters.GT.fGTLinearLength, 0.001f, 1.0f - m_Parameters.GT.fGTLinearStart);

		break;
	}
	default:
		std::unreachable();
		break;
	}

	// Set dirty flag if parameter changed
	auto nMode = std::to_underlying(m_eMode);
	m_bToneMapLUTDirtyFlags[nMode] |= bToneLUTDirty;

	bool bGradingLUTDirty = false;

	ImGui::SeparatorText("White Balance");
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fTemperature", reinterpret_cast<float*>(&m_Parameters.Grading.fTemperature), 0.01f, -1.0f, 1.0f, true, -0.3f, 0.3f, 0.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fTint", reinterpret_cast<float*>(&m_Parameters.Grading.fTint), 0.01f, -1.0f, 1.0f, true, -0.25f, 0.25f, 0.0f);

	ImGui::SeparatorText("Primary Grading");
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3Slope", reinterpret_cast<float*>(&m_Parameters.Grading.v3Slope), 0.01f, 0.0f, 2.0f, true, 0.8f, 1.2f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3Offset", reinterpret_cast<float*>(&m_Parameters.Grading.v3Offset), 0.01f, -1.0f, 1.0f, true, -0.1f, 0.1f, 0.0f);
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3Power", reinterpret_cast<float*>(&m_Parameters.Grading.v3Power), 0.01f, 0.1f, 4.0f, true, 0.8f, 1.2f, 1.0f);

	ImGui::SeparatorText("Global Grading");
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fContrast", reinterpret_cast<float*>(&m_Parameters.Grading.fContrast), 0.01f, 0.0f, 2.0f, true, 0.8f, 1.2f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fContrastPivot", reinterpret_cast<float*>(&m_Parameters.Grading.fContrastPivot), 0.01f, 0.0f, 1.0f, true, 0.35f, 0.65f, 0.5f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fSaturation", reinterpret_cast<float*>(&m_Parameters.Grading.fSaturation), 0.01f, 0.0f, 2.0f, true, 0.7f, 1.3f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fDensity", reinterpret_cast<float*>(&m_Parameters.Grading.fDensity), 0.01f, -2.0f, 2.0f, true, -0.5f, 0.5f, 0.0f);

	ImGui::SeparatorText("Tonal Grading");
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3ShadowTint", reinterpret_cast<float*>(&m_Parameters.Grading.v3ShadowTint), 0.01f, 0.0f, 2.0f, true, 0.7f, 1.3f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fShadowWeight", reinterpret_cast<float*>(&m_Parameters.Grading.fShadowWeight), 0.01f, 0.0f, 2.0f, true, 0.0f, 1.0f, 0.0f);
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3MidtoneTint", reinterpret_cast<float*>(&m_Parameters.Grading.v3MidtoneTint), 0.01f, 0.0f, 2.0f, true, 0.8f, 1.2f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fMidtoneWeight", reinterpret_cast<float*>(&m_Parameters.Grading.fMidtoneWeight), 0.01f, 0.0f, 2.0f, true, 0.0f, 0.8f, 0.0f);
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3HighlightTint", reinterpret_cast<float*>(&m_Parameters.Grading.v3HighlightTint), 0.01f, 0.0f, 2.0f, true, 0.7f, 1.3f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fHighlightWeight", reinterpret_cast<float*>(&m_Parameters.Grading.fHighlightWeight), 0.01f, 0.0f, 2.0f, true, 0.0f, 1.0f, 0.0f);

	ImGui::SeparatorText("Creative final output");
	bGradingLUTDirty |= ShowDragFloat3(cnt++, "v3ColorFilter", reinterpret_cast<float*>(&m_Parameters.Grading.v3ColorFilter), 0.01f, 0.0f, 2.0f, true, 0.8f, 1.2f, 1.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fColorFilterStrength", reinterpret_cast<float*>(&m_Parameters.Grading.fColorFilterStrength), 0.01f, 0.0f, 2.0f, true, 0.0f, 1.0f, 0.0f);
	bGradingLUTDirty |= ShowDragFloat(cnt++, "fBlackLift", reinterpret_cast<float*>(&m_Parameters.Grading.fBlackLift), 0.01f, -1.0f, 1.0f, true, 0.0f, 0.15f, 0.0f);

	// Set dirty flag if parameter changed
	m_bGradingLUTDirtyFlag |= bGradingLUTDirty;
}

void ToneMappingPass::SaveParametersToJson() const
{
	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		SaveAgX();
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		SaveACES();
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		SaveUC2();
		break;
	}
	case TONE_MAPPING_MODE::GT:
	{
		SaveGT();
		break;
	}
	default:
		std::unreachable();
		break;
	}
}

void ToneMappingPass::SaveGrading() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;

	// GradingParameters
	j["fTemperature"] = m_Parameters.Grading.fTemperature;
	j["fTint"] = m_Parameters.Grading.fTint;
	j["fColorFilterStrength"] = m_Parameters.Grading.fColorFilterStrength;
	j["fContrast"] = m_Parameters.Grading.fContrast;
	j["fContrastPivot"] = m_Parameters.Grading.fContrastPivot;
	j["fSaturation"] = m_Parameters.Grading.fSaturation;
	j["fDensity"] = m_Parameters.Grading.fDensity;
	j["fShadowWeight"] = m_Parameters.Grading.fShadowWeight;
	j["fMidtoneWeight"] = m_Parameters.Grading.fMidtoneWeight;
	j["fHighlightWeight"] = m_Parameters.Grading.fHighlightWeight;
	j["fBlackLift"] = m_Parameters.Grading.fBlackLift;


	j["v3Slope"] = { m_Parameters.Grading.v3Slope.x, m_Parameters.Grading.v3Slope.y, m_Parameters.Grading.v3Slope.z };
	j["v3Offset"] = { m_Parameters.Grading.v3Offset.x, m_Parameters.Grading.v3Offset.y, m_Parameters.Grading.v3Offset.z };
	j["v3Power"] = { m_Parameters.Grading.v3Power.x, m_Parameters.Grading.v3Power.y, m_Parameters.Grading.v3Power.z };
	j["v3ShadowTint"] = { m_Parameters.Grading.v3ShadowTint.x, m_Parameters.Grading.v3ShadowTint.y, m_Parameters.Grading.v3ShadowTint.z };
	j["v3MidtoneTint"] = { m_Parameters.Grading.v3MidtoneTint.x, m_Parameters.Grading.v3MidtoneTint.y, m_Parameters.Grading.v3MidtoneTint.z };
	j["v3HighlightTint"] = { m_Parameters.Grading.v3HighlightTint.x, m_Parameters.Grading.v3HighlightTint.y, m_Parameters.Grading.v3HighlightTint.z };
	j["v3ColorFilter"] = { m_Parameters.Grading.v3ColorFilter.x, m_Parameters.Grading.v3ColorFilter.y, m_Parameters.Grading.v3ColorFilter.z };

	std::string strSavePath = std::format("{}/Grading_{}.bin", g_strSavePath, m_strSaveName);
	if (!fs::exists(strSavePath)) {
		fs::path p = strSavePath;
		fs::create_directories(p.parent_path());
	}

	std::ofstream out{ strSavePath, std::ios::binary };
	if (!out) {
		return;
	}

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void ToneMappingPass::SaveAgX() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	j["fExposure"] = m_Parameters.Common.fExposure;
	j["fGamma"] = m_Parameters.Common.fGamma;

	j["fSaturation"] = m_Parameters.Common.fPostSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;
	j["fLookStrength"] = m_Parameters.Common.fGradingStrength;

	// AgXParameters
	j["fAgXWhite"] = m_Parameters.AgX.fAgXWhite;
	j["fAgXBlack"] = m_Parameters.AgX.fAgXBlack;
	j["fAgXContrast"] = m_Parameters.AgX.fAgXContrast;
	j["fAgXMinEV"] = m_Parameters.AgX.fAgXMinEV;
	j["fAgXMaxEV"] = m_Parameters.AgX.fAgXMaxEV;

	std::string strSavePath = std::format("{}/AgX_{}.bin", g_strSavePath, m_strSaveName);
	if (!fs::exists(strSavePath)) {
		fs::path p = strSavePath;
		fs::create_directories(p.parent_path());
	}

	std::ofstream out{ strSavePath, std::ios::binary };
	if (!out) {
		return;
	}

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void ToneMappingPass::SaveGT() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	j["fExposure"] = m_Parameters.Common.fExposure;
	j["fGamma"] = m_Parameters.Common.fGamma;

	j["fSaturation"] = m_Parameters.Common.fPostSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;
	j["fLookStrength"] = m_Parameters.Common.fGradingStrength;

	// GTParameters
	j["fGTMaxBrightness"] = m_Parameters.GT.fGTMaxBrightness;
	j["fGTContrast"] = m_Parameters.GT.fGTContrast;
	j["fGTLinearStart"] = m_Parameters.GT.fGTLinearStart;
	j["fGTLinearLength"] = m_Parameters.GT.fGTLinearLength;
	j["fGTBlack"] = m_Parameters.GT.fGTBlack;
	j["fGTPedestal"] = m_Parameters.GT.fGTPedestal;

	std::string strSavePath = std::format("{}/GT_{}.bin", g_strSavePath, m_strSaveName);
	if (!fs::exists(strSavePath)) {
		fs::path p = strSavePath;
		fs::create_directories(p.parent_path());
	}

	std::ofstream out{ strSavePath, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void ToneMappingPass::SaveUC2() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	j["fExposure"] = m_Parameters.Common.fExposure;
	j["fGamma"] = m_Parameters.Common.fGamma;

	j["fSaturation"] = m_Parameters.Common.fPostSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;
	j["fLookStrength"] = m_Parameters.Common.fGradingStrength;

	// GTParameters
	j["fUC2A"] = m_Parameters.UC2.fUC2A;
	j["fUC2B"] = m_Parameters.UC2.fUC2B;
	j["fUC2C"] = m_Parameters.UC2.fUC2C;
	j["fUC2D"] = m_Parameters.UC2.fUC2D;
	j["fUC2E"] = m_Parameters.UC2.fUC2E;
	j["fUC2F"] = m_Parameters.UC2.fUC2F;
	j["fUC2WhitePoint"] = m_Parameters.UC2.fUC2WhitePoint;
	j["fUC2ExposureBias"] = m_Parameters.UC2.fUC2ExposureBias;

	std::string strSavePath = std::format("{}/UC2_{}.bin", g_strSavePath, m_strSaveName);
	if (!fs::exists(strSavePath)) {
		fs::path p = strSavePath;
		fs::create_directories(p.parent_path());
	}

	std::ofstream out{ strSavePath, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void ToneMappingPass::SaveACES() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	j["fExposure"] = m_Parameters.Common.fExposure;
	j["fGamma"] = m_Parameters.Common.fGamma;
	
	j["fSaturation"] = m_Parameters.Common.fPostSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;
	j["fLookStrength"] = m_Parameters.Common.fGradingStrength;

	// GTParameters
	j["fACESExposureBias"] = m_Parameters.ACES.fACESExposureBias;
	j["fACESPreSaturation"] = m_Parameters.ACES.fACESPreSaturation;
	j["fACESPostSaturation"] = m_Parameters.ACES.fACESPostSaturation;
	j["fACESHighlightDesaturation"] = m_Parameters.ACES.fACESHighlightDesaturation;
	j["fACESCoreOutputScale"] = m_Parameters.ACES.fACESCoreOutputScale;

	std::string strSavePath = std::format("{}/ACES_{}.bin", g_strSavePath, m_strSaveName);
	if (!fs::exists(strSavePath)) {
		fs::path p = strSavePath;
		fs::create_directories(p.parent_path());
	}

	std::ofstream out{ strSavePath, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void ToneMappingPass::LoadParametersFromJson()
{
	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		LoadAgX();
		break;
	}
	case TONE_MAPPING_MODE::GT:
	{
		LoadGT();
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		LoadUC2();
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		LoadACES();
		break;
	}
	default:
		std::unreachable();
		break;
	}

	m_bToneMapLUTDirtyFlags[std::to_underlying(m_eMode)] = true;
}

void ToneMappingPass::LoadGrading()
{
	std::string strParametersPath = std::format("{}/Grading_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Grading.fTemperature  = inJson["fTemperature"].get<float>();
	m_Parameters.Grading.fTint  = inJson["fTint"].get<float>();
	m_Parameters.Grading.fColorFilterStrength  = inJson["fColorFilterStrength"].get<float>();
	m_Parameters.Grading.fContrast  = inJson["fContrast"].get<float>();
	m_Parameters.Grading.fContrastPivot  = inJson["fContrastPivot"].get<float>();
	m_Parameters.Grading.fSaturation  = inJson["fSaturation"].get<float>();
	m_Parameters.Grading.fDensity  = inJson["fDensity"].get<float>();
	m_Parameters.Grading.fShadowWeight  = inJson["fShadowWeight"].get<float>();
	m_Parameters.Grading.fMidtoneWeight  = inJson["fMidtoneWeight"].get<float>();
	m_Parameters.Grading.fHighlightWeight  = inJson["fHighlightWeight"].get<float>();
	m_Parameters.Grading.fBlackLift  = inJson["fBlackLift"].get<float>();

	std::vector<float> f;

	f = inJson["v3Slope"].get<std::vector<float>>();
	m_Parameters.Grading.v3Slope = Vector3(f.data());

	f = inJson["v3Offset"].get<std::vector<float>>();
	m_Parameters.Grading.v3Offset = Vector3(f.data());

	f = inJson["v3Power"].get<std::vector<float>>();
	m_Parameters.Grading.v3Power = Vector3(f.data());

	f = inJson["v3ShadowTint"].get<std::vector<float>>();
	m_Parameters.Grading.v3ShadowTint = Vector3(f.data());

	f = inJson["v3MidtoneTint"].get<std::vector<float>>();
	m_Parameters.Grading.v3MidtoneTint = Vector3(f.data());

	f = inJson["v3HighlightTint"].get<std::vector<float>>();
	m_Parameters.Grading.v3HighlightTint = Vector3(f.data());

	f = inJson["v3ColorFilter"].get<std::vector<float>>();
	m_Parameters.Grading.v3ColorFilter = Vector3(f.data());

	m_bGradingLUTDirtyFlag = true;
}

void ToneMappingPass::LoadAgX()
{
	std::string strParametersPath = std::format("{}/AgX_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Common.fExposure = inJson["fExposure"].get<float>();
	m_Parameters.Common.fGamma = inJson["fGamma"].get<float>();
	m_Parameters.Common.fPostSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();
	m_Parameters.Common.fGradingStrength = inJson["fLookStrength"].get<float>();

	m_Parameters.AgX.fAgXWhite = inJson["fAgXWhite"].get<float>();
	m_Parameters.AgX.fAgXBlack = inJson["fAgXBlack"].get<float>();
	m_Parameters.AgX.fAgXContrast = inJson["fAgXContrast"].get<float>();
	m_Parameters.AgX.fAgXMinEV = inJson["fAgXMinEV"].get<float>();
	m_Parameters.AgX.fAgXMaxEV = inJson["fAgXMaxEV"].get<float>();
}

void ToneMappingPass::LoadGT()
{
	std::string strParametersPath = std::format("{}/GT_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Common.fExposure = inJson["fExposure"].get<float>();
	m_Parameters.Common.fGamma = inJson["fGamma"].get<float>();
	m_Parameters.Common.fPostSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();
	m_Parameters.Common.fGradingStrength = inJson["fLookStrength"].get<float>();

	m_Parameters.GT.fGTMaxBrightness = inJson["fGTMaxBrightness"].get<float>();
	m_Parameters.GT.fGTContrast = inJson["fGTContrast"].get<float>();
	m_Parameters.GT.fGTLinearStart = inJson["fGTLinearStart"].get<float>();
	m_Parameters.GT.fGTLinearLength = inJson["fGTLinearLength"].get<float>();
	m_Parameters.GT.fGTBlack = inJson["fGTBlack"].get<float>();
	m_Parameters.GT.fGTPedestal = inJson["fGTPedestal"].get<float>();
}

void ToneMappingPass::LoadUC2()
{
	std::string strParametersPath = std::format("{}/UC2_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Common.fExposure = inJson["fExposure"].get<float>();
	m_Parameters.Common.fGamma = inJson["fGamma"].get<float>();
	m_Parameters.Common.fPostSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();
	m_Parameters.Common.fGradingStrength = inJson["fLookStrength"].get<float>();

	m_Parameters.UC2.fUC2A = inJson["fUC2A"].get<float>();
	m_Parameters.UC2.fUC2B = inJson["fUC2B"].get<float>();
	m_Parameters.UC2.fUC2C = inJson["fUC2C"].get<float>();
	m_Parameters.UC2.fUC2D = inJson["fUC2D"].get<float>();
	m_Parameters.UC2.fUC2E = inJson["fUC2E"].get<float>();
	m_Parameters.UC2.fUC2F = inJson["fUC2F"].get<float>();
	m_Parameters.UC2.fUC2WhitePoint = inJson["fUC2WhitePoint"].get<float>();
	m_Parameters.UC2.fUC2ExposureBias = inJson["fUC2ExposureBias"].get<float>();
}

void ToneMappingPass::LoadACES()
{
	std::string strParametersPath = std::format("{}/ACES_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Common.fExposure = inJson["fExposure"].get<float>();
	m_Parameters.Common.fGamma = inJson["fGamma"].get<float>();
	m_Parameters.Common.fPostSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();
	m_Parameters.Common.fGradingStrength = inJson["fLookStrength"].get<float>();

	m_Parameters.ACES.fACESExposureBias = inJson["fACESExposureBias"].get<float>();
	m_Parameters.ACES.fACESPreSaturation = inJson["fACESPreSaturation"].get<float>();
	m_Parameters.ACES.fACESPostSaturation = inJson["fACESPostSaturation"].get<float>();
	m_Parameters.ACES.fACESHighlightDesaturation = inJson["fACESHighlightDesaturation"].get<float>();
	m_Parameters.ACES.fACESCoreOutputScale = inJson["fACESCoreOutputScale"].get<float>();
}










//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImGui Helper

bool ToneMappingPass::ShowDragFloat(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault, const char* cstrformat)
{
	bool bResult = ImGui::DragFloat(cstrLabel, v, fSpeed, fMin, fMax, cstrformat);
	ImGui::SameLine();

	if (bShowHelp) {
		std::string strHelp = std::format("Recommanded range : {} ~ {}\n Default = {}", fRecommandMin, fRecommandMax, fDefault);
		GuiManager::HelpMarker(strHelp.c_str());
	}

	std::string strButton = std::format("Default{}", cnt++);
	bool bReset = ImGui::Button(strButton.c_str());
	if (bReset) *v = fDefault;

	return bResult || bReset;
}

bool ToneMappingPass::ShowDragFloat3(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault)
{
	bool bResult = ImGui::DragFloat3(cstrLabel, v, fSpeed, fMin, fMax);
	ImGui::SameLine();

	if (bShowHelp) {
		std::string strHelp = std::format("Recommanded range : {} ~ {}\n Default = {}", fRecommandMin, fRecommandMax, fDefault);
		GuiManager::HelpMarker(strHelp.c_str());
	}

	std::string strButton = std::format("Default{}", cnt++);
	bool bReset = ImGui::Button(strButton.c_str());
	if (bReset) {
		Vector3* pv3Value = reinterpret_cast<Vector3*>(v);
		*pv3Value = Vector3(fDefault, fDefault, fDefault);
	}

	return bResult || bReset;
}
