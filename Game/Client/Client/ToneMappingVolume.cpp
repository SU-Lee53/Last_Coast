#include "pch.h"
#include "ToneMappingVolume.h"

ToneMappingVolume::ToneMappingVolume()
	:m_eCurrentToneMapper(TONE_MAPPING_MODE::ACES)
{
	m_Parameters.Common = ToneMappingParameter::g_DefaultCommonParameters;
	m_Parameters.Grading = ToneMappingParameter::g_DefaultGradingParameters;
	m_Parameters.ACES = ToneMappingParameter::g_DefaultACESParameters;
}

void ToneMappingVolume::LoadFromFiles(const std::string& strFilename)
{
	LoadParametersFromBinary(strFilename);
}

void ToneMappingVolume::Update()
{
	if (m_bSkipThisFrame) {
		m_bSkipThisFrame = false;
		return;
	}
	m_unDirtyFlag = 0;
}

CB_TONE_MAPPING_COMMON_DATA ToneMappingVolume::GetCommonCBData() const
{
	return CB_TONE_MAPPING_COMMON_DATA{
		.fExposure = m_Parameters.Common.fExposure,
		.fTargetLuminance = m_Parameters.Common.fTargetLuminance,
		.fMinExposure = m_Parameters.Common.fMinExposure,
		.fMaxExposure = m_Parameters.Common.fMaxExposure,
		.nEnableAutoExposure = m_Parameters.Common.nEnableAutoExposure,
		.fGamma = m_Parameters.Common.fGamma,
		.fSaturation = m_Parameters.Common.fPostSaturation,
		.fInputScale = m_Parameters.Common.fInputScale,
		.fOutputScale = m_Parameters.Common.fOutputScale,
		.fGradingStrength = m_Parameters.Common.fGradingStrength,
		.pad = Vector2(0.f),
	};
}

CB_TONE_MAPPING_LUT_DATA ToneMappingVolume::GetToneMapperLUTCBData() const
{
	CB_TONE_MAPPING_LUT_DATA data{};
	data.gToneMappingCommon0.x = static_cast<float>(m_eCurrentToneMapper);
	switch (m_eCurrentToneMapper)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.AgX, sizeof(AgXParameters));
		break;
	}
	case TONE_MAPPING_MODE::ACES:
	{
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.ACES, sizeof(ACESParameters));
		break;
	}
	case TONE_MAPPING_MODE::UC2:
	{
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.UC2, sizeof(UC2Parameters));
		break;
	}
	case TONE_MAPPING_MODE::GT:
	{
		::memcpy(&data.gToneMappingCommon0.y, &m_Parameters.GT, sizeof(GTParameters));
		break;
	}
	default:
		std::unreachable();
		break;
	}

	return data;
}

CB_TONE_MAPPING_LUT_DATA ToneMappingVolume::GetGradingLUTCBData() const
{
	CB_TONE_MAPPING_LUT_DATA data{};
	::memcpy(&data, &m_Parameters.Grading, sizeof(GradingParameters));
	return data;
}

void ToneMappingVolume::SetDirtyFlag(uint8 eFlag, bool bValue)
{
	(bValue) ? m_unDirtyFlag |= eFlag : m_unDirtyFlag &= ~eFlag;
}

bool ToneMappingVolume::IsDirty(uint8 eFlag) const
{
	return (m_unDirtyFlag & eFlag);
}

void ToneMappingVolume::ShowDebugOptions()
{
	TONE_MAPPING_MODE eBefore = m_eCurrentToneMapper;
	bool bModeChanged = ImGui::SliderInt("Mode", reinterpret_cast<int32*>(&m_eCurrentToneMapper), 0, std::to_underlying(TONE_MAPPING_MODE::COUNT) - 1, m_cstrModeName[std::to_underlying(m_eCurrentToneMapper)]);

	if (bModeChanged) {
		SetDefaultParameters(eBefore, m_eCurrentToneMapper);

		uint32 unBeforeMode = std::to_underlying(eBefore);
		uint32 unAfterMode = std::to_underlying(m_eCurrentToneMapper);

		m_bSkipThisFrame |= true;
		SetDirtyFlag((LUT_DIRTY_FLAG::TONE_MAPPER | LUT_DIRTY_FLAG::GRADING), true);
	}

	const uint32 unMode = std::to_underlying(m_eCurrentToneMapper);

	if (ImGui::Button("Reset Parameters")) {
		SetDefaultParameters(TONE_MAPPING_MODE::UNDEFINED, m_eCurrentToneMapper);

		m_bSkipThisFrame |= true;
		SetDirtyFlag((LUT_DIRTY_FLAG::TONE_MAPPER | LUT_DIRTY_FLAG::GRADING), true);
	}

	ImGui::InputText("Save/Load name", &m_strSaveName);

	if (ImGui::Button("Save Parameters To Binary")) {
		SaveParametersToBinary(m_strSaveName);
	}

	ImGui::SameLine();
	if (ImGui::Button("Load Parameters From Binary")) {
		LoadParametersFromBinary(m_strSaveName);

		m_bSkipThisFrame |= true;
		SetDirtyFlag((LUT_DIRTY_FLAG::TONE_MAPPER | LUT_DIRTY_FLAG::GRADING), true);
	}

	//ImGui::Text("Last ToneMap LUT Updated : %f", m_fLastToneLUTUpdated);
	//ImGui::Text("Last Grading LUT Updated : %f", m_fLastLookLUTUpdated);

	int cnt{};

	ImGui::SeparatorText("Global / Outputs");

	ImGui::SliderInt("nEnableAutoExposure", reinterpret_cast<int*>(&m_Parameters.Common.nEnableAutoExposure), 1, 0, (m_Parameters.Common.nEnableAutoExposure == 0 ? "ON" : "OFF"));
	ShowDragFloat(cnt++, "fExposure", reinterpret_cast<float*>(&m_Parameters.Common.fExposure), 0.01f, 0.f, 4.f, true, 0.f, 2.f, 1.f);
	ShowDragFloat(cnt++, "fTargetLuminance", reinterpret_cast<float*>(&m_Parameters.Common.fTargetLuminance), 0.01f, 0.1f, 0.3f, true, 0.1f, 0.3f, 0.18f);
	ShowDragFloat(cnt++, "fMinExposure", reinterpret_cast<float*>(&m_Parameters.Common.fMinExposure), 0.01f, 0.03f, 0.10f, true, 0.03f, 0.10f, 0.05f);
	ShowDragFloat(cnt++, "fMaxExposure", reinterpret_cast<float*>(&m_Parameters.Common.fMaxExposure), 0.01f, 1.f, 32.f, true, 8.f, 32.f, 16.f);

	ShowDragFloat(cnt++, "fGamma", reinterpret_cast<float*>(&m_Parameters.Common.fGamma), 0.01f, 1.0f, 3.0f, true, 2.0f, 2.4f, 2.2f);
	ShowDragFloat(cnt++, "fSaturation", reinterpret_cast<float*>(&m_Parameters.Common.fPostSaturation), 0.01f, 0.f, 2.f, true, 0.75f, 1.15, 1.f);
	ShowDragFloat(cnt++, "fInputScale", reinterpret_cast<float*>(&m_Parameters.Common.fInputScale), 0.01f, 0.25f, 4.f, true, 0.5f, 2.0f, 1.f);
	ShowDragFloat(cnt++, "fOutputScale", reinterpret_cast<float*>(&m_Parameters.Common.fOutputScale), 0.01f, 0.5f, 2.f, true, 0.8, 1.2, 1.f);
	ShowDragFloat(cnt++, "fGradingStrength", reinterpret_cast<float*>(&m_Parameters.Common.fGradingStrength), 0.01f, 0.0f, 1.0f, true, 0.0f, 1.0f, 1.0f);

	bool bToneLUTDirty = false;

	ImGui::SeparatorText("Tone mapper core parameters");

	switch (m_eCurrentToneMapper)
	{
	case TONE_MAPPING_MODE::AGX:
	{
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXWhite", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXWhite), 0.001f, 0.8f, 1.2f, true, 0.9f, 1.05f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXBlack", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXBlack), 0.001f, 0.0f, 0.15f, true, 0.0f, 0.05f, 0.f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXContrast", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXContrast), 0.0001f, 0.6f, 1.4f, true, 0.85f, 1.15f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXMinEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMinEV), 0.001f, -16.0f, -8.0f, true, -13.5f, -10.f, -12.47393f, "%.8f");
		bToneLUTDirty |= ShowDragFloat(cnt++, "fAgXMaxEV", reinterpret_cast<float*>(&m_Parameters.AgX.fAgXMaxEV), 0.001f, 2.0f, 8.0f, true, 3.f, 5.f, 4.026069f, "%.8f");

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
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTMaxBrightness", reinterpret_cast<float*>(&m_Parameters.GT.fGTMaxBrightness), 0.001f, 0.8f, 2.0f, true, 0.9f, 1.3f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTContrast", reinterpret_cast<float*>(&m_Parameters.GT.fGTContrast), 0.001f, 0.5f, 2.0f, true, 0.85f, 1.25f, 1.0f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTLinearStart", reinterpret_cast<float*>(&m_Parameters.GT.fGTLinearStart), 0.001f, 0.0f, 0.5f, true, 0.12f, 0.3f, 0.22f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTLinearLength", reinterpret_cast<float*>(&m_Parameters.GT.fGTLinearLength), 0.0001f, 0.05f, 0.8f, true, 0.2f, 0.55f, 0.4f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTBlack", reinterpret_cast<float*>(&m_Parameters.GT.fGTBlack), 0.001f, 0.5f, 2.5f, true, 1.f, 1.6f, 1.33f);
		bToneLUTDirty |= ShowDragFloat(cnt++, "fGTPedestal", reinterpret_cast<float*>(&m_Parameters.GT.fGTPedestal), 0.00001f, 0.0f, 0.1f, true, 0.0f, 0.03f, 0.0f);

		m_Parameters.GT.fGTLinearStart = std::clamp(m_Parameters.GT.fGTLinearStart, 0.0f, 1.0f);

		m_Parameters.GT.fGTLinearLength = std::clamp(m_Parameters.GT.fGTLinearLength, 0.001f, 1.0f - m_Parameters.GT.fGTLinearStart);

		break;
	}
	default:
		std::unreachable();
		break;
	}

	// Set dirty flag if parameter changed
	if (bToneLUTDirty) {
		m_bSkipThisFrame |= bToneLUTDirty;
		SetDirtyFlag(LUT_DIRTY_FLAG::TONE_MAPPER, bToneLUTDirty);
	}

	bool bGradingLUTDirty = false;
	{
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
	}

	// Set dirty flag if parameter changed
	if (bGradingLUTDirty) {
		m_bSkipThisFrame |= bGradingLUTDirty;
		SetDirtyFlag(LUT_DIRTY_FLAG::GRADING, bGradingLUTDirty);
	}
}

void ToneMappingVolume::SetDefaultParameters(TONE_MAPPING_MODE eModeBefore, TONE_MAPPING_MODE eModeAfter)
{
	if (eModeBefore == eModeAfter) {
		return;
	}

	switch (eModeAfter)
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




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Save / Load

void ToneMappingVolume::SaveParametersToBinary(const std::string& strFilename) const
{
	std::ofstream out{ g_strSavePath + strFilename + ".bin", std::ios::binary};
	out << m_Parameters;
}

void ToneMappingVolume::LoadParametersFromBinary(const std::string& strFilename)
{
	std::ifstream in{ g_strSavePath + strFilename + ".bin", std::ios::binary };
	in >> m_Parameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImGui Helper

bool ToneMappingVolume::ShowDragFloat(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault, const char* cstrformat)
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

bool ToneMappingVolume::ShowDragFloat3(int cnt, const char* cstrLabel, float* v, float fSpeed, float fMin, float fMax, bool bShowHelp, float fRecommandMin, float fRecommandMax, float fDefault)
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
