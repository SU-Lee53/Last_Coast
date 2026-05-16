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

void PostProcessingVolume::Update()
{

}

void PostProcessingVolume::ShowDebugOptions()
{
	ImGui::SeparatorText("Bloom");
	{
		ImGui::DragFloat("Threshold", &m_Bloom.fThreshold, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Soft Knee", &m_Bloom.fSoftKnee, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Intensity", &m_Bloom.fIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Radius", &m_Bloom.fRadius, 0.01f, 0.25f, 4.0f);
	}

	if (ImGui::Button("Reset Bloom Parameters")) {
		m_Bloom.fThreshold = 1.0f;
		m_Bloom.fSoftKnee = 0.5f;
		m_Bloom.fIntensity = 0.6f;
		m_Bloom.fRadius = 1.0f;
	}

}
