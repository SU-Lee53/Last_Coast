#include "pch.h"
#include "Light.h"

LightData PointLight::MakeCBData()
{
	return LightData{
		.v3Color = m_v3Color,
		.fIntensity = m_fIntensity,
		.v3Position = m_v3Position,
		.v3Attenuation = Vector3{m_fAttenuation0, m_fAttenuation1, m_fAttenuation2},
		.bEnable = (m_bEnable) ? 1u : 0u,
		.nType = std::to_underlying(LIGHT_TYPE::POINT_LIGHT),
		.fRange = m_fRange,
	};
}

void PointLight::ShowControllImGui()
{
	ImGui::Text("Light Type : Point Light");

	if (ImGui::Button((m_bEnable) ? "ON" : "OFF")) {
		m_bEnable = !m_bEnable;
	}

	ImGui::DragFloat3("Position", (float*)&m_v3Position, 100.f, -100000.f, 100000.f);
	ImGui::DragFloat3("Color", (float*)&m_v3Color, 0.1f, 0.f, 1.f);
	ImGui::DragFloat("Intensity", (float*)&m_fIntensity, 0.1f, 0.f, 100.f);

	ImGui::DragFloat("Range", (float*)&m_fRange, 0.1f, 0.f, 100.f);
	ImGui::DragFloat("Attenuation0", (float*)&m_fAttenuation0, 1.0f, 0.f, 1000.f);
	ImGui::DragFloat("Attenuation1", (float*)&m_fAttenuation1, 1.0f, 0.f, 1000.f);
	ImGui::DragFloat("Attenuation2", (float*)&m_fAttenuation2, 1.0f, 0.f, 1000.f);
}

LightData SpotLight::MakeCBData()
{
	return LightData{
		.v3Color = m_v3Color,
		.fIntensity = m_fIntensity,
		.v3Position = m_v3Position,
		.fFalloff = m_fFalloff,
		.v3Direction = m_v3Direction,
		.fTheta = m_fTheta,
		.v3Attenuation = Vector3{m_fAttenuation0, m_fAttenuation1, m_fAttenuation2},
		.fPhi = m_fPhi,
		.bEnable = (m_bEnable) ? 1u : 0u,
		.nType = std::to_underlying(LIGHT_TYPE::SPOT_LIGHT),
		.fRange = m_fRange,
		.pad0 = 0.f
	};
}

void SpotLight::ShowControllImGui()
{
	ImGui::Text("Light Type : Spot Light");

	if (ImGui::Button((m_bEnable) ? "ON" : "OFF")) {
		m_bEnable = !m_bEnable;
	}

	ImGui::DragFloat3("Position", (float*)&m_v3Position, 100.f, -100000.f, 100000.f);
	ImGui::DragFloat3("Direction", (float*)&m_v3Direction, 1.0, -180.f, 180.f);
	ImGui::DragFloat3("Color", (float*)&m_v3Color, 0.1f, 0.f, 1.f);
	ImGui::DragFloat("Intensity", (float*)&m_fIntensity, 0.1f, 0.f, 100.f);

	ImGui::DragFloat("Range", (float*)&m_fRange, 0.1f, 0.f, 100.f);
	ImGui::DragFloat("FallOff", (float*)&m_fFalloff, 1.0, 0.f, 100.f);

	ImGui::Text("Degree : %f", XMConvertToDegrees(std::acosf(m_fTheta)));
	ImGui::DragFloat("Theta(Inner Cone)", (float*)&m_fTheta, 0.1, -1, 1);
	
	ImGui::Text("Degree : %f", XMConvertToDegrees(std::acosf(m_fPhi)));
	ImGui::DragFloat("Phi(Outer Cone)", (float*)&m_fPhi, 0.1, -1, 1);

	ImGui::DragFloat("Attenuation0", (float*)&m_fAttenuation0, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Attenuation1", (float*)&m_fAttenuation1, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Attenuation2", (float*)&m_fAttenuation2, 0.1f, 0.f, 1000.f);

}

LightData DirectionalLight::MakeCBData()
{
	return LightData {
		.v3Color = m_v3Color,
		.fIntensity = m_fIntensity,
		.v3Position = m_v3Position,
		.v3Direction = m_v3Direction,
		.bEnable = (m_bEnable) ? 1u : 0u,
		.nType = std::to_underlying(LIGHT_TYPE::DIRECTIONAL_LIGHT),
	};
}

void DirectionalLight::ShowControllImGui()
{
	ImGui::Text("Light Type : Directional Light");
	ImGui::DragFloat3("Position", (float*)&m_v3Position, 100.f, -100000.f, 100000.f);
	ImGui::DragFloat3("Color", (float*)&m_v3Color, 0.1f, 0.f, 1.f);
	ImGui::DragFloat("Intensity", (float*)&m_fIntensity, 0.1f, 0.f, 100.f);
}
