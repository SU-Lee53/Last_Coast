#include "pch.h"
#include "PostProcessingVolume.h"

CB_BLOOM_DATA PostProcessingVolume::GetBloomCBData(XMINT2 xmi2InputSize, XMINT2 xmi2OutputSize) const
{
	return CB_BLOOM_DATA{
		.gBloomThreshold = m_Bloom.fThreshold,
		.gBloomSoftKnee = m_Bloom.fSoftKnee,
		.gBloomIntensity = m_Bloom.fIntensity,
		.gBloomRadius = m_Bloom.fRadius,
		.gInputSize = xmi2InputSize,
		.gOutputSize = xmi2OutputSize,
	};
}

CB_SSAO_DATA PostProcessingVolume::GetSSAOCBData() const
{
	return CB_SSAO_DATA{
		.gfRadius = m_SSAO.fRadius,
		.gfBias = m_SSAO.fBias,
		.gfPower = m_SSAO.fPower,
		.gfIntensity = m_SSAO.fIntensity,
		.gnSampleCount = m_SSAO.nSampleCount,
		.gfDepthSigma = m_SSAO.fDepthSigma,
		.gfNormalSigma = m_SSAO.fNormalSigma,
		.gfNoiseScale = m_SSAO.fNoiseScale,
	};
}

CB_SCREEN_FX_DATA PostProcessingVolume::GetScreenFXCBData() const
{
	return CB_SCREEN_FX_DATA{
		.gGrainStrength = m_ScreenFX.fGrainStrength ,
		.gGrainScale = m_ScreenFX.fGrainScale ,
		.gVignetteStrength = m_ScreenFX.fVignetteStrength ,
		.gVignetteRadius = m_ScreenFX.fVignetteRadius ,
		.gfVignetteSoftness = m_ScreenFX.fVignetteSoftness ,
	};
}

CB_LIGHT_SHAFT_DATA PostProcessingVolume::GetLightShaftCBData(const Vector2& v2LightScreenPosition) const
{
	return CB_LIGHT_SHAFT_DATA{
		.gv2LightScreenPosition = v2LightScreenPosition,
		.gfIntensity = m_LightShaft.fIntensity,
		.gfDecay = m_LightShaft.fDecay,
		.gfDensity = m_LightShaft.fDensity,
		.gfWeight = m_LightShaft.fWeight,
		.gfExposure = m_LightShaft.fExposure,
		.gfDepthThreshold = m_LightShaft.fDepthThreshold,
		.gnSampleCount = m_LightShaft.nSampleCount,
		.gnEnable = (m_LightShaft.bEnable) ? 1 : 0,
	};
}

void PostProcessingVolume::Update()
{

}

void PostProcessingVolume::ShowDebugOptions()
{
	ImGui::SeparatorText("Bloom");
	{
		ImGui::DragFloat("Bloom Threshold", &m_Bloom.fThreshold, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Bloom Soft Knee", &m_Bloom.fSoftKnee, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Bloom Intensity", &m_Bloom.fIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Bloom Radius", &m_Bloom.fRadius, 0.01f, 0.25f, 4.0f);
	}

	if (ImGui::Button("Reset Bloom Parameters")) {
		m_Bloom = BloomParameters{};
	}
	
	ImGui::SeparatorText("SSAO");
	{
		ImGui::DragFloat("SSAO Radius", &m_SSAO.fRadius);
		ImGui::DragFloat("SSAO Bias", &m_SSAO.fBias);
		ImGui::DragFloat("SSAO Power", &m_SSAO.fPower);
		ImGui::DragFloat("SSAO Intensity", &m_SSAO.fIntensity);
		ImGui::DragInt("SSAO SampleCount", &m_SSAO.nSampleCount);
		ImGui::DragFloat("SSAO DepthSigma", &m_SSAO.fDepthSigma);
		ImGui::DragFloat("SSAO NormalSigma", &m_SSAO.fNormalSigma);
		ImGui::DragFloat("SSAO NoiseScale", &m_SSAO.fNoiseScale);
	}

	if (ImGui::Button("Reset SSAO Parameters")) {
		m_SSAO = SSAOParameters{};
	}

	ImGui::SeparatorText("Light Shaft");
	{
		ImGui::Checkbox("Light Shaft Enable", &m_LightShaft.bEnable);
		ImGui::DragFloat("Light Shaft Intensity", &m_LightShaft.fIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Light Shaft Decay", &m_LightShaft.fDecay, 0.001f, 0.8f, 1.0f);
		ImGui::DragFloat("Light Shaft Density", &m_LightShaft.fDensity, 0.01f, 0.1f, 2.0f);
		ImGui::DragFloat("Light Shaft Weight", &m_LightShaft.fWeight, 0.001f, 0.001f, 0.5f);
		ImGui::DragFloat("Light Shaft Exposure", &m_LightShaft.fExposure, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Light Shaft DepthThreshold", &m_LightShaft.fDepthThreshold, 0.0001f, 0.95f, 1.0f, "%.4f");
		ImGui::DragInt("Light Shaft SampleCount", &m_LightShaft.nSampleCount, 1, 8, 96);
	}

	if (ImGui::Button("Reset Light Shaft Parameters")) {
		m_LightShaft = LightShaftParameters{};
	}
	
	ImGui::SeparatorText("Screen FX");
	{
		ImGui::SeparatorText("Grain");
		{
			ImGui::DragFloat("Grain Strength", &m_ScreenFX.fGrainStrength, 0.01f, 0.0f, 0.08f);
			ImGui::DragFloat("Grain Scale", &m_ScreenFX.fGrainScale, 0.01f, 0.25f, 4.0f);
		}

		ImGui::SeparatorText("Vignette");
		{
			ImGui::DragFloat("Vignette Strength", &m_ScreenFX.fVignetteStrength, 0.01f, 0.f, 1.f);
			ImGui::DragFloat("Vignette Radius", &m_ScreenFX.fVignetteRadius, 0.01f, 0.2f, 1.5f);
			ImGui::DragFloat("Vignette Softness", &m_ScreenFX.fVignetteSoftness, 0.01f, 0.01f, 1.5f);
		}
	}

	if (ImGui::Button("Reset Screen FX Parameters")) {
		m_ScreenFX = ScreenFXParameters{};
	}

}
