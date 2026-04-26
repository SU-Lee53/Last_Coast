#include "pch.h"
#include "DefferedFogPass.h"

void DefferedFogPass::Initialize()
{
	m_cbFogData = g_DefaultParameters;
	CreatePipelineState();
}

void DefferedFogPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	// Update Height Fog base height

	// Set Render Targets
	auto pHDRRenderTarget = std::static_pointer_cast<RenderTargetTexture>(RENDER->GetCurrentHDRBuffer(1).GetResource());
	pHDRRenderTarget->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Clear Render Targets
	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pHDRRenderTarget->GetRTVHandle();
	pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, NULL);

	//CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = RENDER->GetDepthStencilBufferHandle();
	//pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);

	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);
}

void DefferedFogPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	constexpr auto rootParamFog = std::to_underlying(ROOT_PARAMETER::FOG_DATA);
	
	auto fogCBuffer = RENDER->AllocCBuffer<CB_FOG_DATA>();
	fogCBuffer.WriteData(&m_cbFogData);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamFog, fogCBuffer.GPUAddress);

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(pd3dCommandList, 1);
}

void DefferedFogPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void DefferedFogPass::ShowDebugInfo()
{
	if (ImGui::Button("Reset Parameters")) {
		m_cbFogData = g_DefaultParameters;
	}
	
	ImGui::InputText("Save Name", &m_strSaveName);
	if (ImGui::Button("Save Parameters")) {
		SaveParametersToJson();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Parameters")) {
		LoadParametersFromJson();
	}
	
	float fHeightOffset = 0.f;

	ImGui::DragFloat4("FogColor", reinterpret_cast<float*>(&m_cbFogData.gfogColor), 0.01f, 0.f, 1.f);

	ImGui::DragFloat("fFogStartDistance", reinterpret_cast<float*>(&m_cbFogData.gfFogStartDistance), 1.f, 0.f, std::numeric_limits<float>::max());
	ImGui::DragFloat("fFogCutOffDistance", reinterpret_cast<float*>(&m_cbFogData.gfFogCutOffDistance), 1.f, 0.f, std::numeric_limits<float>::max());
	ImGui::DragFloat("fFogDistanceDensity", reinterpret_cast<float*>(&m_cbFogData.gfFogDistanceDensity), 0.0001f, 0.0001f, 0.05f, "%.4f");
	ImGui::DragFloat("FogDistancePower", reinterpret_cast<float*>(&m_cbFogData.gFogDistancePower), 0.01f, 0.25f, 4.0f);

	ImGui::DragFloat("fFogHeightDensity", reinterpret_cast<float*>(&m_cbFogData.gfFogHeightDensity), 0.001f, 0.f, 0.2f);
	ImGui::DragFloat("fFogHeightFalloff", reinterpret_cast<float*>(&m_cbFogData.gfFogHeightFalloff), 0.001f, 0.001, 3.0f);
	ImGui::DragFloat("fFogBaseHeight(Offet from player y)", reinterpret_cast<float*>(&fHeightOffset), 0.1f);
	ImGui::Text("fFogBaseHeight : %f", m_cbFogData.gfFogBaseHeight);
	ImGui::DragFloat("fFogHeightStartDistance", reinterpret_cast<float*>(&m_cbFogData.gfFogHeightStartDistance), 0.1f, 0.f, std::numeric_limits<float>::max());

	ImGui::DragFloat("fFogMaxOpacity", reinterpret_cast<float*>(&m_cbFogData.gfFogMaxOpacity), 0.01f, 0.f, 1.f);

	float fHeightBase = CUR_SCENE->GetPlayer()->GetTransform()->GetPosition().y;
	m_cbFogData.gfFogBaseHeight = fHeightBase + fHeightOffset;
}

void DefferedFogPass::CreatePipelineState()
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
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("DefferedFogVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("DefferedFogPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;

		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}

void DefferedFogPass::SaveParametersToJson() const
{
	using namespace nlohmann;

	json j;
	j["fogColor"] = { m_cbFogData.gfogColor.x,  m_cbFogData.gfogColor.y,  m_cbFogData.gfogColor.z,  m_cbFogData.gfogColor.w };
	j["fFogStartDistance"] = m_cbFogData.gfFogStartDistance;
	j["fFogCutOffDistance"] = m_cbFogData.gfFogCutOffDistance;
	j["fFogDistanceDensity"] = m_cbFogData.gfFogDistanceDensity;
	j["FogDistancePower"] = m_cbFogData.gFogDistancePower;
	j["fFogHeightDensity"] = m_cbFogData.gfFogHeightDensity;
	j["fFogHeightFalloff"] = m_cbFogData.gfFogHeightFalloff;
	j["fFogBaseHeight"] = m_cbFogData.gfFogBaseHeight;
	j["fFogHeightStartDistance"] = m_cbFogData.gfFogHeightStartDistance;
	j["fFogMaxOpacity"] = m_cbFogData.gfFogMaxOpacity;

	std::string strSavePath = std::format("{}/fog_{}.bin", g_strSavePath, m_strSaveName);
	std::ofstream out{ strSavePath, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}

void DefferedFogPass::LoadParametersFromJson()
{
	std::string strParametersPath = std::format("{}/fog_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_cbFogData.gfFogStartDistance = inJson["fFogStartDistance"].get<float>();
	m_cbFogData.gfFogCutOffDistance = inJson["fFogCutOffDistance"].get<float>();
	m_cbFogData.gfFogDistanceDensity = inJson["fFogDistanceDensity"].get<float>();
	m_cbFogData.gFogDistancePower = inJson["FogDistancePower"].get<float>();
	m_cbFogData.gfFogHeightDensity = inJson["fFogHeightDensity"].get<float>();
	m_cbFogData.gfFogHeightFalloff = inJson["fFogHeightFalloff"].get<float>();
	m_cbFogData.gfFogBaseHeight = inJson["fFogBaseHeight"].get<float>();
	m_cbFogData.gfFogHeightStartDistance = inJson["fFogHeightStartDistance"].get<float>();
	m_cbFogData.gfFogMaxOpacity = inJson["fFogMaxOpacity"].get<float>();

	std::vector<float> f;
	f = inJson["fogColor"].get<std::vector<float>>();
	m_cbFogData.gfogColor = Vector4(f.data());

}
