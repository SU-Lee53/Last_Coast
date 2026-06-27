#include "pch.h"
#include "PostProcessingVolume.h"

CB_BLOOM_DATA PostProcessingVolume::GetBloomCBData(XMINT2 xmi2InputSize, XMINT2 xmi2OutputSize) const
{
	return CB_BLOOM_DATA{
		.gBloomThreshold = m_Parameters.Bloom.fThreshold,
		.gBloomSoftKnee = m_Parameters.Bloom.fSoftKnee,
		.gBloomIntensity = m_Parameters.Bloom.fIntensity,
		.gBloomRadius = m_Parameters.Bloom.fRadius,
		.gInputSize = xmi2InputSize,
		.gOutputSize = xmi2OutputSize,
	};
}

CB_SSAO_DATA PostProcessingVolume::GetSSAOCBData() const
{
	return CB_SSAO_DATA{
		.gfRadius = m_Parameters.SSAO.fRadius,
		.gfBias = m_Parameters.SSAO.fBias,
		.gfPower = m_Parameters.SSAO.fPower,
		.gfIntensity = m_Parameters.SSAO.fIntensity,
		.gnSampleCount = m_Parameters.SSAO.nSampleCount,
		.gfDepthSigma = m_Parameters.SSAO.fDepthSigma,
		.gfNormalSigma = m_Parameters.SSAO.fNormalSigma,
		.gfNoiseScale = m_Parameters.SSAO.fNoiseScale,
	};
}

CB_SCREEN_FX_DATA PostProcessingVolume::GetScreenFXCBData() const
{
	return CB_SCREEN_FX_DATA{
		.gGrainStrength = m_Parameters.ScreenFX.fGrainStrength ,
		.gGrainScale = m_Parameters.ScreenFX.fGrainScale ,
		.gVignetteStrength = m_Parameters.ScreenFX.fVignetteStrength ,
		.gVignetteRadius = m_Parameters.ScreenFX.fVignetteRadius ,
		.gfVignetteSoftness = m_Parameters.ScreenFX.fVignetteSoftness ,
	};
}

CB_LIGHT_SHAFT_DATA PostProcessingVolume::GetLightShaftCBData(const Vector2& v2LightScreenPosition) const
{
	return CB_LIGHT_SHAFT_DATA{
		.gv2LightScreenPosition = v2LightScreenPosition,
		.gfIntensity = m_Parameters.LightShaft.fIntensity,
		.gfDecay = m_Parameters.LightShaft.fDecay,
		.gfDensity = m_Parameters.LightShaft.fDensity,
		.gfWeight = m_Parameters.LightShaft.fWeight,
		.gfExposure = m_Parameters.LightShaft.fExposure,
		.gfDepthThreshold = m_Parameters.LightShaft.fDepthThreshold,
		.gnSampleCount = m_Parameters.LightShaft.nSampleCount,
		.gnEnable = (m_Parameters.LightShaft.bEnable) ? 1 : 0,
	};
}

CB_FOG_DATA PostProcessingVolume::GetFogCBData() const
{
	float fFogBaseHeight = m_Parameters.Fog.fFogBaseHeightOffset;

	if (const auto& pPlayer = CUR_SCENE->GetPlayer()) {
		fFogBaseHeight += pPlayer->GetTransform()->GetPosition().y;
	}

	return CB_FOG_DATA{
		.gfogColor = m_Parameters.Fog.v4FogColor,
		.gfFogStartDistance = m_Parameters.Fog.fFogStartDistance,
		.gfFogCutOffDistance = m_Parameters.Fog.fFogCutOffDistance,
		.gfFogDistanceDensity = m_Parameters.Fog.fFogDistanceDensity,
		.gFogDistancePower = m_Parameters.Fog.fFogDistancePower,
		.gfFogHeightDensity = m_Parameters.Fog.fFogHeightDensity,
		.gfFogHeightFalloff = m_Parameters.Fog.fFogHeightFalloff,
		.gfFogBaseHeight = fFogBaseHeight,
		.gfFogHeightStartDistance = m_Parameters.Fog.fFogHeightStartDistance,
		.gfFogMaxOpacity = m_Parameters.Fog.fFogMaxOpacity,
	};
}

void PostProcessingVolume::Update()
{

}

bool PostProcessingVolume::SaveParametersToBinary(const std::string& strSaveName) const
{
	std::ofstream out{ g_strSavePath + strSaveName + ".bin", std::ios::binary };
	if (!out) {
		return false;
	}

	out << m_Parameters;
	return !!out;
}

bool PostProcessingVolume::LoadParametersFromBinary(const std::string& strSaveName)
{
	std::ifstream in{ g_strSavePath + strSaveName + ".bin", std::ios::binary };
	if (!in) {
		return false;
	}

	in >> m_Parameters;
	return !!in;
}

void PostProcessingVolume::ShowDebugOptions()
{
	ImGui::InputText("Save Name", &m_strSaveName);
	if (ImGui::Button("Save Post Processing Parameters")) {
		SaveParametersToBinary(m_strSaveName);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Post Processing Parameters")) {
		LoadParametersFromBinary(m_strSaveName);
	}

	if (ImGui::Button("Reset All Post Processing Parameters")) {
		m_Parameters = PostProcessingParameters{};
	}

	ImGui::SeparatorText("Bloom");
	{
		ImGui::DragFloat("Bloom Threshold", &m_Parameters.Bloom.fThreshold, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Bloom Soft Knee", &m_Parameters.Bloom.fSoftKnee, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Bloom Intensity", &m_Parameters.Bloom.fIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Bloom Radius", &m_Parameters.Bloom.fRadius, 0.01f, 0.25f, 4.0f);
	}

	if (ImGui::Button("Reset Bloom Parameters")) {
		m_Parameters.Bloom = BloomParameters{};
	}
	
	ImGui::SeparatorText("SSAO");
	{
		ImGui::DragFloat("SSAO Radius", &m_Parameters.SSAO.fRadius);
		ImGui::DragFloat("SSAO Bias", &m_Parameters.SSAO.fBias);
		ImGui::DragFloat("SSAO Power", &m_Parameters.SSAO.fPower);
		ImGui::DragFloat("SSAO Intensity", &m_Parameters.SSAO.fIntensity);
		ImGui::DragInt("SSAO SampleCount", &m_Parameters.SSAO.nSampleCount);
		ImGui::DragFloat("SSAO DepthSigma", &m_Parameters.SSAO.fDepthSigma);
		ImGui::DragFloat("SSAO NormalSigma", &m_Parameters.SSAO.fNormalSigma);
		ImGui::DragFloat("SSAO NoiseScale", &m_Parameters.SSAO.fNoiseScale);
	}

	if (ImGui::Button("Reset SSAO Parameters")) {
		m_Parameters.SSAO = SSAOParameters{};
	}

	ImGui::SeparatorText("Light Shaft");
	{
		ImGui::Checkbox("Light Shaft Enable", &m_Parameters.LightShaft.bEnable);
		ImGui::DragFloat("Light Shaft Intensity", &m_Parameters.LightShaft.fIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Light Shaft Decay", &m_Parameters.LightShaft.fDecay, 0.001f, 0.8f, 1.0f);
		ImGui::DragFloat("Light Shaft Density", &m_Parameters.LightShaft.fDensity, 0.01f, 0.1f, 2.0f);
		ImGui::DragFloat("Light Shaft Weight", &m_Parameters.LightShaft.fWeight, 0.001f, 0.001f, 0.5f);
		ImGui::DragFloat("Light Shaft Exposure", &m_Parameters.LightShaft.fExposure, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Light Shaft DepthThreshold", &m_Parameters.LightShaft.fDepthThreshold, 0.0001f, 0.95f, 1.0f, "%.4f");
		ImGui::DragInt("Light Shaft SampleCount", &m_Parameters.LightShaft.nSampleCount, 1, 8, 96);
	}

	if (ImGui::Button("Reset Light Shaft Parameters")) {
		m_Parameters.LightShaft = LightShaftParameters{};
	}

	ImGui::SeparatorText("Fog");
	{
		ImGui::DragFloat4("Fog Color", reinterpret_cast<float*>(&m_Parameters.Fog.v4FogColor), 0.01f, 0.0f, 1.0f);

		ImGui::DragFloat("Fog Start Distance", &m_Parameters.Fog.fFogStartDistance, 1.0f, 0.0f, std::numeric_limits<float>::max());
		ImGui::DragFloat("Fog CutOff Distance", &m_Parameters.Fog.fFogCutOffDistance, 1.0f, 0.0f, std::numeric_limits<float>::max());
		ImGui::DragFloat("Fog Distance Density", &m_Parameters.Fog.fFogDistanceDensity, 0.0001f, 0.0001f, 0.05f, "%.4f");
		ImGui::DragFloat("Fog Distance Power", &m_Parameters.Fog.fFogDistancePower, 0.01f, 0.25f, 4.0f);

		ImGui::DragFloat("Fog Height Density", &m_Parameters.Fog.fFogHeightDensity, 0.001f, 0.0f, 0.2f);
		ImGui::DragFloat("Fog Height Falloff", &m_Parameters.Fog.fFogHeightFalloff, 0.001f, 0.001f, 3.0f);
		ImGui::DragFloat("Fog Base Height Offset", &m_Parameters.Fog.fFogBaseHeightOffset, 0.1f);
		ImGui::Text("Fog Base Height : %f", GetFogCBData().gfFogBaseHeight);
		ImGui::DragFloat("Fog Height Start Distance", &m_Parameters.Fog.fFogHeightStartDistance, 0.1f, 0.0f, std::numeric_limits<float>::max());

		ImGui::DragFloat("Fog Max Opacity", &m_Parameters.Fog.fFogMaxOpacity, 0.01f, 0.0f, 1.0f);
	}

	if (ImGui::Button("Reset Fog Parameters")) {
		m_Parameters.Fog = FogParameters{};
	}
	
	ImGui::SeparatorText("Screen FX");
	{
		ImGui::SeparatorText("Grain");
		{
			ImGui::DragFloat("Grain Strength", &m_Parameters.ScreenFX.fGrainStrength, 0.01f, 0.0f, 0.08f);
			ImGui::DragFloat("Grain Scale", &m_Parameters.ScreenFX.fGrainScale, 0.01f, 0.25f, 4.0f);
		}

		ImGui::SeparatorText("Vignette");
		{
			ImGui::DragFloat("Vignette Strength", &m_Parameters.ScreenFX.fVignetteStrength, 0.01f, 0.f, 1.f);
			ImGui::DragFloat("Vignette Radius", &m_Parameters.ScreenFX.fVignetteRadius, 0.01f, 0.2f, 1.5f);
			ImGui::DragFloat("Vignette Softness", &m_Parameters.ScreenFX.fVignetteSoftness, 0.01f, 0.01f, 1.5f);
		}
	}

	if (ImGui::Button("Reset Screen FX Parameters")) {
		m_Parameters.ScreenFX = ScreenFXParameters{};
	}

}
