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
