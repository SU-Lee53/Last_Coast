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
	CB_SSAO_DATA data;
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
		m_Bloom.fThreshold = 1.0f;
		m_Bloom.fSoftKnee = 0.5f;
		m_Bloom.fIntensity = 0.6f;
		m_Bloom.fRadius = 1.0f;
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
		m_SSAO.fRadius = 3.0f;
		m_SSAO.fBias = 0.001f;
		m_SSAO.fPower = 1.5f;
		m_SSAO.fIntensity = 1.0f;
		m_SSAO.nSampleCount = 16;
		m_SSAO.fDepthSigma = 120.0f;
		m_SSAO.fNormalSigma = 16.0f;
		m_SSAO.fNoiseScale = 1.0f;
	}

}
