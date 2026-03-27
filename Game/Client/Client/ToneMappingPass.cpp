#include "pch.h"
#include "ToneMappingPass.h"

void ToneMappingPass::Initialize()
{
	SetDefaultParameters(TONE_MAPPING_MODE::UNDEFINED, TONE_MAPPING_MODE::AGX);
	CreatePipelineState();
}

void ToneMappingPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	constexpr auto rootParamToneMapping = std::to_underlying(ROOT_PARAMETER::TONE_MAPPING_DATA);
	
	CB_TONE_MAPPING_DATA data = MakeCBData();
	auto paramCBuffer = RENDER->AllocCBuffer<CB_TONE_MAPPING_DATA>();
	paramCBuffer.WriteData(&data);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamToneMapping, paramCBuffer.GPUAddress);

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);
}

void ToneMappingPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(pd3dCommandList, 1);
}

void ToneMappingPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
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
}

CB_TONE_MAPPING_DATA ToneMappingPass::MakeCBData() const
{
	CB_TONE_MAPPING_DATA data;
	data.nMode = std::to_underlying(m_eMode);
	::memcpy(&data.gToneMappingCommon0.x, &m_Parameters.Common, sizeof(ToneMappingCommonParameters));

	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		::memcpy(&data.gToneMappingCommon1.z, &m_Parameters.AgX, sizeof(AgXParameters));
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		::memcpy(&data.gToneMappingCommon1.z, &m_Parameters.UC2, sizeof(UC2Parameters));
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{

		break;
	}
	default:
		std::unreachable();
		break;
	}

	::memcpy(&data.v3Slope, &m_Parameters.Look, sizeof(LookParameters));


	return data;
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
	case TONE_MAPPING_MODE::UC2:
	{
		m_Parameters.UC2 = ToneMappingParameter::g_DefaultUC2Parameters;
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		break;
	}
	default:
		std::unreachable();
		break;
	}

	m_Parameters.Look = ToneMappingParameter::g_DefaultLookParameters;

}

void ToneMappingPass::ShowDragFloat(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault, const char* cstrformat)
{
	ImGui::DragFloat(cstrLabel, v, fSpeed, fMin, fMax, cstrformat);
	ImGui::SameLine();

	if (bShowHelp) {
		std::string strHelp = std::format("Recommanded range : {} ~ {}\n Default = {}", fRecommandMin, fRecommandMax, fDefault);
		GuiManager::HelpMarker(strHelp.c_str());
	}

	std::string strButton = std::format("Default{}", cnt++);
	if (ImGui::Button(strButton.c_str())) *v = fDefault;
}

void ToneMappingPass::ShowDragFloat3(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault)
{
	ImGui::DragFloat3(cstrLabel, v, fSpeed, fMin, fMax);
	ImGui::SameLine();

	if (bShowHelp) {
		std::string strHelp = std::format("Recommanded range : {} ~ {}\n Default = {}", fRecommandMin, fRecommandMax, fDefault);
		GuiManager::HelpMarker(strHelp.c_str());
	}

	std::string strButton = std::format("Default{}", cnt++);
	if (ImGui::Button(strButton.c_str())) {
		Vector3* pv3Value = reinterpret_cast<Vector3*>(v);
		*pv3Value = Vector3(fDefault, fDefault, fDefault);
	}
}

void ToneMappingPass::ShowDebugInfo()
{
	uint32 unMode = std::to_underlying(m_eMode);
	TONE_MAPPING_MODE eBefore = m_eMode;
	ImGui::SliderInt("Mode", reinterpret_cast<int*>(&m_eMode), 0, 2, g_cstrModeName[unMode]);
	SetDefaultParameters(eBefore, m_eMode);

	if (ImGui::Button("Reset Parameters")) {
		SetDefaultParameters(TONE_MAPPING_MODE::UNDEFINED, m_eMode);
	}

	ImGui::InputText("Save/Load name", &m_strSaveName);

	if (ImGui::Button(std::format("Save {} Parameters", g_cstrModeName[unMode]).c_str())) {
		SaveParametersToJson();
	}

	ImGui::SameLine();
	if (ImGui::Button(std::format("Load {} Parameters", g_cstrModeName[unMode]).c_str())) {
		LoadParametersFromJson();
	}

	if (ImGui::Button(std::format("Save Look Parameters", g_cstrModeName[unMode]).c_str())) {
		SaveLook();
	}

	ImGui::SameLine();
	if (ImGui::Button(std::format("Load Look Parameters", g_cstrModeName[unMode]).c_str())) {
		LoadLook();
	}

	int cnt{};

	ImGui::SeparatorText("Global / Outputs");

	ShowDragFloat(cnt++, "fExposure", reinterpret_cast<float*>(&m_Parameters.Common.fExposure), 0.01f, 0.f, 4.f, true, 0.f, 2.f, 1.f);
	ShowDragFloat(cnt++, "fGamma", reinterpret_cast<float*>(&m_Parameters.Common.fGamma), 0.01f, 1.0f, 3.0f, true, 2.0f, 2.4f, 2.2f);

	ShowDragFloat(cnt++, "fSaturation", reinterpret_cast<float*>(&m_Parameters.Common.fSaturation), 0.01f, 0.f, 2.f, true, 0.75f, 1.15, 1.f);
	ShowDragFloat(cnt++, "fInputScale", reinterpret_cast<float*>(&m_Parameters.Common.fInputScale), 0.01f, 0.25f, 4.f, true, 0.5f, 2.0f, 1.f);
	ShowDragFloat(cnt++, "fOutputScale", reinterpret_cast<float*>(&m_Parameters.Common.fOutputScale), 0.01f, 0.5f, 2.f, true, 0.8, 1.2, 1.f);

	switch (m_eMode)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		ShowDragFloat(cnt++, "fAgXWhite", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXWhite),		0.001f, 0.8f, 1.2f, true, 0.9f, 1.05f, 1.0f);
		ShowDragFloat(cnt++, "fAgXBlack", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXBlack),		0.001f, 0.0f, 0.15f, true, 0.0f, 0.05f, 0.f);
		ShowDragFloat(cnt++, "fAgXContrast", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXContrast),	0.0001f, 0.6f, 1.4f, true, 0.85f, 1.15f, 1.0f);
		ShowDragFloat(cnt++, "fAgXMinEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMinEV),		0.000001f, -16.0f, -8.0f, true, -13.5f, -10.f, -12.47393f, "%.8f");
		ShowDragFloat(cnt++, "fAgXMaxEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMaxEV),		0.000001f, 2.0f, 8.0f, true, 3.f, 5.f, 4.026069f, "%.8f");

		if (m_Parameters.AgX.fAgXMaxEV < m_Parameters.AgX.fAgXMinEV) {
			m_Parameters.AgX.fAgXMaxEV = std::max(m_Parameters.AgX.fAgXMaxEV, m_Parameters.AgX.fAgXMinEV + 1.0f);
		}

		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		ShowDragFloat(cnt++, "fUC2MaxBrightness", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2MaxBrightness),	0.001f, 0.8f, 2.0f, true, 0.9f, 1.3f, 1.0f);
		ShowDragFloat(cnt++, "fUC2Contrast", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2Contrast),				0.001f, 0.5f, 2.0f, true, 0.85f, 1.25f, 1.0f);
		ShowDragFloat(cnt++, "fUC2LinearStart", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2LinearStart),		0.001f, 0.0f, 0.5f, true, 0.12f, 0.3f, 0.22f);
		ShowDragFloat(cnt++, "fUC2LinearLength", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2LinearLength),		0.0001f, 0.05f, 0.8f, true, 0.2f, 0.55f, 0.4f);
		ShowDragFloat(cnt++, "fUC2Black", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2Black),					0.001f, 0.5f, 2.5f, true, 1.f, 1.6f, 1.33f);
		ShowDragFloat(cnt++, "fUC2Pedestal", reinterpret_cast<float*>(&m_Parameters.UC2.fUC2Pedestal),				0.00001f, 0.0f, 0.1f, true, 0.0f, 0.03f, 0.0f);

		if (m_Parameters.UC2.fUC2LinearStart + m_Parameters.UC2.fUC2LinearLength <= 1.0f) {
			m_Parameters.UC2.fUC2LinearLength = std::min(m_Parameters.UC2.fUC2LinearLength, 1.0f - m_Parameters.UC2.fUC2LinearStart);
		}

		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{

		break;
	}
	default:
		std::unreachable();
		break;
	}



	ImGui::SeparatorText("LookParameters");
	ImGui::SeparatorText("Color modify");
	ShowDragFloat3(cnt++, "v3Slope", reinterpret_cast<float*>(&m_Parameters.Look.v3Slope), 0.001f, 0.5f, 1.5f, true, 0.9f, 1.1f, 1.0f);
	ShowDragFloat3(cnt++, "v3Offset", reinterpret_cast<float*>(&m_Parameters.Look.v3Offset), 0.001f, -0.25f, 0.25f, true, -0.05f, 0.05f, 0.0f);
	ShowDragFloat3(cnt++, "v3Power", reinterpret_cast<float*>(&m_Parameters.Look.v3Power), 0.001f, 0.5f, 1.5f, true, 0.9f, 1.12f, 1.0f);

	ImGui::SeparatorText("Sectional hues");
	ShowDragFloat3(cnt++, "v3ShadowTint", reinterpret_cast<float*>(&m_Parameters.Look.v3ShadowTint), 0.001f, 0.f, 1.5f, true, 0.85f, 1.10f, 1.0f);
	ShowDragFloat(cnt++, "fShadowTintStrength", reinterpret_cast<float*>(&m_Parameters.Look.fShadowTintStrength), 0.001f, 0.0f, 1.0f, true, 0.0f, 0.35f, 0.1f);
	ShowDragFloat(cnt++, "fShadowStartLuma", reinterpret_cast<float*>(&m_Parameters.Look.fShadowStartLuma), 0.001f, 0.0f, 0.4f, true, 0.f, 0.f, 0.1f);
	ShowDragFloat(cnt++, "fShadowEndLuma", reinterpret_cast<float*>(&m_Parameters.Look.fShadowEndLuma), 0.001f, 0.1f, 0.8f, true, 0.f, 0.f, 0.55f);

	m_Parameters.Look.fShadowEndLuma = std::max(m_Parameters.Look.fShadowStartLuma + 0.01f, m_Parameters.Look.fShadowEndLuma);

	ShowDragFloat3(cnt++, "v3HighlightTint", reinterpret_cast<float*>(&m_Parameters.Look.v3HighlightTint), 0.001f, 0.0f, 1.5f, true, 0.9f, 1.1f, 1.0f);
	ShowDragFloat(cnt++, "fHighlightTintStrength", reinterpret_cast<float*>(&m_Parameters.Look.fHighlightTintStrength), 0.01f, 0.0f, 1.0f, true, 0.0f, 0.35f, 0.2f);
	ShowDragFloat(cnt++, "fHighlightStartLuma", reinterpret_cast<float*>(&m_Parameters.Look.fHighlightStartLuma), 0.001f, 0.2f, 0.8f, true, 0.f, 0.f, 0.45f);
	ShowDragFloat(cnt++, "fHighlightEndLuma", reinterpret_cast<float*>(&m_Parameters.Look.fHighlightEndLuma), 0.001f, 0.4f, 1.0f, true, 0.f, 0.f, 0.85f);

	m_Parameters.Look.fHighlightEndLuma = std::max(m_Parameters.Look.fHighlightStartLuma + 0.01f, m_Parameters.Look.fHighlightEndLuma);

	ImGui::SeparatorText("Tone Structure");
	ShowDragFloat(cnt++, "fContrastPivot", reinterpret_cast<float*>(&m_Parameters.Look.fContrastPivot), 0.001f, 0.0f, 1.0f, true, 0.18f, 0.5f, 0.4f);
	ShowDragFloat(cnt++, "fContrastStrength", reinterpret_cast<float*>(&m_Parameters.Look.fContrastStrength), 0.001f, 0.5f, 1.5f, true, 0.85f, 1.2f, 1.0f);
	ShowDragFloat(cnt++, "fBlackLift", reinterpret_cast<float*>(&m_Parameters.Look.fBlackLift), 0.001f, 0.0f, 0.2, true, 0.f, 0.06f, 0.02f);
	ShowDragFloat(cnt++, "fDensity", reinterpret_cast<float*>(&m_Parameters.Look.fDensity), 0.01f, 0.0f, 1.5f, true, 0.0f, 0.35f, 0.1f);


	ImGui::SeparatorText("Tone Structure");
	ShowDragFloat(cnt++, "fLookSaturation", reinterpret_cast<float*>(&m_Parameters.Look.fLookSaturation), 0.01f, 0.0f, 2.0f, true, 0.8f, 1.2f, 1.0f);
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
	case TONE_MAPPING_MODE::UC2:
	{
		SaveUC2();
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		SaveACES();
		break;
	}
	default:
		std::unreachable();
		break;
	}
}

void ToneMappingPass::SaveLook() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	// LookParameters
	j["fContrastPivot"] = m_Parameters.Look.fContrastPivot;
	j["fContrastStrength"] = m_Parameters.Look.fContrastStrength;
	j["fBlackLift"] = m_Parameters.Look.fBlackLift;
	j["fShadowTintStrength"] = m_Parameters.Look.fShadowTintStrength;
	j["fHighlightTintStrength"] = m_Parameters.Look.fHighlightTintStrength;
	j["fDensity"] = m_Parameters.Look.fDensity;
	j["fLookSaturation"] = m_Parameters.Look.fLookSaturation;

	j["fShadowStartLuma"] = m_Parameters.Look.fShadowStartLuma;
	j["fShadowEndLuma"] = m_Parameters.Look.fShadowEndLuma;

	j["fHighlightStartLuma"] = m_Parameters.Look.fHighlightStartLuma;
	j["fHighlightEndLuma"] = m_Parameters.Look.fHighlightEndLuma;

	j["v3Slope"] = { m_Parameters.Look.v3Slope.x, m_Parameters.Look.v3Slope.y, m_Parameters.Look.v3Slope.z };
	j["v3Offset"] = { m_Parameters.Look.v3Offset.x, m_Parameters.Look.v3Offset.y, m_Parameters.Look.v3Offset.z };
	j["v3Power"] = { m_Parameters.Look.v3Power.x, m_Parameters.Look.v3Power.y, m_Parameters.Look.v3Power.z };
	j["v3ShadowTint"] = { m_Parameters.Look.v3ShadowTint.x, m_Parameters.Look.v3ShadowTint.y, m_Parameters.Look.v3ShadowTint.z };
	j["v3HighlightTint"] = { m_Parameters.Look.v3HighlightTint.x, m_Parameters.Look.v3HighlightTint.y, m_Parameters.Look.v3HighlightTint.z };

	std::string strSavePath = std::format("{}/Look_{}.bin", g_strSavePath, m_strSaveName);
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

	j["fSaturation"] = m_Parameters.Common.fSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;

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

void ToneMappingPass::SaveUC2() const
{
	using namespace nlohmann;
	namespace fs = std::filesystem;

	json j;
	j["fExposure"] = m_Parameters.Common.fExposure;
	j["fGamma"] = m_Parameters.Common.fGamma;

	j["fSaturation"] = m_Parameters.Common.fSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;

	// UC2Parameters
	j["fUC2MaxBrightness"] = m_Parameters.UC2.fUC2MaxBrightness;
	j["fUC2Contrast"] = m_Parameters.UC2.fUC2Contrast;
	j["fUC2LinearStart"] = m_Parameters.UC2.fUC2LinearStart;
	j["fUC2LinearLength"] = m_Parameters.UC2.fUC2LinearLength;
	j["fUC2Black"] = m_Parameters.UC2.fUC2Black;
	j["fUC2Pedestal"] = m_Parameters.UC2.fUC2Pedestal;

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
	
	j["fSaturation"] = m_Parameters.Common.fSaturation;
	j["fInputScale"] = m_Parameters.Common.fInputScale;
	j["fOutputScale"] = m_Parameters.Common.fOutputScale;

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
}

void ToneMappingPass::LoadLook()
{
	std::string strParametersPath = std::format("{}/Look_{}.bin", g_strSavePath, m_strSaveName);
	std::ifstream inFile{ strParametersPath, std::ios::binary };
	if (!inFile) {
		return;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json inJson = nlohmann::json::from_bson(bson);;

	m_Parameters.Look.fLookStrength = inJson["fLookStrength"].get<float>();

	m_Parameters.Look.fContrastPivot = inJson["fContrastPivot"].get<float>();
	m_Parameters.Look.fContrastStrength = inJson["fContrastStrength"].get<float>();
	m_Parameters.Look.fBlackLift = inJson["fBlackLift"].get<float>();
	m_Parameters.Look.fShadowTintStrength = inJson["fShadowTintStrength"].get<float>();
	m_Parameters.Look.fHighlightTintStrength = inJson["fHighlightTintStrength"].get<float>();
	m_Parameters.Look.fDensity = inJson["fDensity"].get<float>();
	m_Parameters.Look.fLookSaturation = inJson["fLookSaturation"].get<float>();

	m_Parameters.Look.fShadowStartLuma = inJson["fShadowStartLuma"].get<float>();
	m_Parameters.Look.fShadowEndLuma = inJson["fShadowEndLuma"].get<float>();
	m_Parameters.Look.fHighlightStartLuma = inJson["fHighlightStartLuma"].get<float>();
	m_Parameters.Look.fHighlightEndLuma = inJson["fHighlightEndLuma"].get<float>();

	std::vector<float> f;
	f = inJson["v3Slope"].get<std::vector<float>>();
	m_Parameters.Look.v3Slope = Vector3(f.data());

	f = inJson["v3Offset"].get<std::vector<float>>();
	m_Parameters.Look.v3Offset = Vector3(f.data());

	f = inJson["v3Power"].get<std::vector<float>>();
	m_Parameters.Look.v3Power = Vector3(f.data());

	f = inJson["v3ShadowTint"].get<std::vector<float>>();
	m_Parameters.Look.v3ShadowTint = Vector3(f.data());

	f = inJson["v3HighlightTint"].get<std::vector<float>>();
	m_Parameters.Look.v3HighlightTint = Vector3(f.data());
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
	m_Parameters.Common.fSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();

	m_Parameters.AgX.fAgXWhite = inJson["fAgXWhite"].get<float>();
	m_Parameters.AgX.fAgXBlack = inJson["fAgXBlack"].get<float>();
	m_Parameters.AgX.fAgXContrast = inJson["fAgXContrast"].get<float>();
	m_Parameters.AgX.fAgXMinEV = inJson["fAgXMinEV"].get<float>();
	m_Parameters.AgX.fAgXMaxEV = inJson["fAgXMaxEV"].get<float>();
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
	m_Parameters.Common.fSaturation = inJson["fSaturation"].get<float>();
	m_Parameters.Common.fInputScale = inJson["fInputScale"].get<float>();
	m_Parameters.Common.fOutputScale = inJson["fOutputScale"].get<float>();

	m_Parameters.UC2.fUC2MaxBrightness = inJson["fUC2MaxBrightness"].get<float>();
	m_Parameters.UC2.fUC2Contrast = inJson["fUC2Contrast"].get<float>();
	m_Parameters.UC2.fUC2LinearStart = inJson["fUC2LinearStart"].get<float>();
	m_Parameters.UC2.fUC2LinearLength = inJson["fUC2LinearLength"].get<float>();
	m_Parameters.UC2.fUC2Black = inJson["fUC2Black"].get<float>();
	m_Parameters.UC2.fUC2Pedestal = inJson["fUC2Pedestal"].get<float>();
}

void ToneMappingPass::LoadACES()
{
}
