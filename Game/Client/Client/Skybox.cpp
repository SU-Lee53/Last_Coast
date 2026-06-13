#include "pch.h"
#include "Skybox.h"
#include "ComputePass.h"


void Skybox::Initialize()
{
	// Initialize parameters
	std::string strParametersPath = g_strSkyboxBasePath + "SkyboxParameters.bin";
	if (std::filesystem::exists(strParametersPath)) {
		std::ifstream inFile{ strParametersPath, std::ios::binary };
		std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
		nlohmann::json inJson = nlohmann::json::from_bson(bson);;

		m_fDayNightBlend		= inJson.value("fDayNightBlend", 1.0f);
		m_fTimeOfDayHours		= inJson.value("fTimeOfDayHours", std::clamp(m_fDayNightBlend, 0.0f, 1.0f) * 12.0f);
		m_fSunIntensity			= inJson["fSunIntensity"].get<float>();
		m_fMoonIntensity		= inJson["fMoonIntensity"].get<float>();
		m_fDirectionalLightIntensityScale = inJson.value("fDirectionalLightIntensityScale", 0.04f);
		m_fAmbientIntensity		= inJson.value("fAmbientIntensity", 0.08f);
		m_fSunDiskSize			= inJson["fSunDiskSize"].get<float>();
		m_fMoonDiskSize			= inJson["fMoonDiskSize"].get<float>();
		m_fSunGlowSize			= inJson["fSunGlowSize"].get<float>();
		m_fMoonGlowSize			= inJson["fMoonGlowSize"].get<float>();
		m_fTwilightWidth		= inJson["fTwilightWidth"].get<float>();
		m_fTwilightIntensity	= inJson["fTwilightIntensity"].get<float>();
		m_fTwilightSunFocus		= inJson["fTwilightSunFocus"].get<float>();
		m_fCloudCoverage		= inJson["fCloudCoverage"].get<float>();
		m_fCloudDensity			= inJson["fCloudDensity"].get<float>();
		m_fCloudSpeed			= inJson["fCloudSpeed"].get<float>();
		m_fSkyIntensity			= inJson["fSkyIntensity"].get<float>();
		m_fCloudScale			= inJson["fCloudScale"].get<float>();
		m_fCloudLightIntensity	= inJson["fCloudLightIntensity"].get<float>();
		m_fStarDensity			= inJson["fStarDensity"].get<float>();
		m_fStarScale			= inJson["fStarScale"].get<float>();

		std::vector<float> f;
		f = inJson["v3NoonHorizontalDirection"].get<std::vector<float>>();
		m_v3NoonHorizontalDirection = Vector3(f.data());
		
		f = inJson["v3TwilightColor"].get<std::vector<float>>();
		m_v3TwilightColor = Vector3(f.data());

		f = inJson["v3SunColor"].get<std::vector<float>>();
		m_v3SunColor = Vector3(f.data());

		f = inJson["v3MoonColor"].get<std::vector<float>>();
		m_v3MoonColor = Vector3(f.data());

		f = inJson["v3DayZenithColor"].get<std::vector<float>>();
		m_v3DayZenithColor = Vector3(f.data());

		f = inJson["v3DayHorizonColor"].get<std::vector<float>>();
		m_v3DayHorizonColor = Vector3(f.data());

		f = inJson["v3NightZenithColor"].get<std::vector<float>>();
		m_v3NightZenithColor = Vector3(f.data());

		f = inJson["v3NightHorizonColor"].get<std::vector<float>>();
		m_v3NightHorizonColor = Vector3(f.data());

	}
	else {
		m_v3NoonHorizontalDirection = Vector3{ 0.35f, 0.0f, 1.0f };

		m_fDayNightBlend = 1.0f;
		m_fTimeOfDayHours = 12.0f;
		m_v3SunDirection = Vector3{ 0.f, 0.f, 0.f };

		m_fSunIntensity = 20.f;
		m_fMoonIntensity = 1.2f;
		m_fDirectionalLightIntensityScale = 0.04f;
		m_fAmbientIntensity = 0.08f;
		m_fSunDiskSize = 0.995f;
		m_fMoonDiskSize = 0.9985f;

		m_fSunGlowSize = 0.9975f;
		m_fMoonGlowSize = 0.9988f;
		m_fTwilightWidth = 0.15f;
		m_fTwilightIntensity = 1.5f;

		m_fTwilightSunFocus = 3.f;
		m_v3TwilightColor = Vector3{ 1.0f, 0.38f, 0.12f };

		m_fCloudCoverage = 0.f;
		m_fCloudDensity = 0.f;
		m_fCloudSpeed = 0.f;
		m_fSkyIntensity = 1.f;

		m_fCloudScale = 1.0f;
		m_fCloudLightIntensity = 1.0f;
		m_fStarDensity = 1.0f;
		m_fStarScale = 1.0f;

		m_v3DayZenithColor = { 0.18f, 0.45f, 0.95f };
		m_v3DayHorizonColor = { 0.75f, 0.85f, 1.00f };
		m_v3NightZenithColor = { 0.01f, 0.02f, 0.06f };
		m_v3NightHorizonColor = { 0.04f, 0.05f, 0.10f };
	}

	m_fTwilightWidth = GetEffectiveTwilightWidth(m_fTwilightWidth);
	UpdateTimeDerivedValues();
	SyncSunLightWithScene();
}

void Skybox::Update()
{
	m_fTimeOfDayHours = NormalizeTimeOfDayHours(m_fTimeOfDayHours);
	UpdateTimeDerivedValues();
	SyncSunLightWithScene();
}

void Skybox::UpdateTimeDerivedValues()
{
	Vector3 v3NoonDir = m_v3NoonHorizontalDirection;
	v3NoonDir.y = 0.f;
	if (v3NoonDir.LengthSquared() <= 1e-6f) {
		v3NoonDir = Vector3::Backward;
	}
	v3NoonDir.Normalize();

	const float fSolarPhase = (m_fTimeOfDayHours - 6.0f) / 24.0f * XM_2PI;

	Vector3 v3SunDir = v3NoonDir * std::cos(fSolarPhase) + Vector3::Up * std::sin(fSolarPhase);
	v3SunDir.Normalize();

	m_v3SunDirection = v3SunDir;

	const float fTwilightWidth = GetEffectiveTwilightWidth(m_fTwilightWidth);
	const float fDayBlendT = std::clamp((m_v3SunDirection.y + fTwilightWidth) / (fTwilightWidth * 2.0f), 0.0f, 1.0f);
	m_fDayNightBlend = fDayBlendT * fDayBlendT * (3.0f - 2.0f * fDayBlendT);
}

void Skybox::SyncSunLightWithScene() const
{
	const auto pSunLight = CUR_SCENE->GetSunLight();
	if (!pSunLight) {
		return;
	}

	const float fDayBlend = std::clamp(m_fDayNightBlend, 0.0f, 1.0f);
	const Vector3 v3VisibleLightDirection = (fDayBlend >= 0.5f) ? m_v3SunDirection : -m_v3SunDirection;
	const Vector3 v3LightColor = Vector3::Lerp(m_v3MoonColor, m_v3SunColor, fDayBlend);
	const float fSkyLightIntensity = std::lerp(m_fMoonIntensity, m_fSunIntensity, fDayBlend);

	pSunLight->m_v3Direction = -v3VisibleLightDirection;
	pSunLight->m_v3Direction.Normalize();
	pSunLight->m_v3Color = v3LightColor;
	pSunLight->m_fIntensity = fSkyLightIntensity * m_fDirectionalLightIntensityScale;

	const float fTwilight = 1.0f - std::abs(fDayBlend * 2.0f - 1.0f);
	Vector3 v3AmbientColor = Vector3::Lerp(m_v3NightHorizonColor, m_v3DayHorizonColor, fDayBlend);
	v3AmbientColor += m_v3TwilightColor * (fTwilight * m_fTwilightIntensity * 0.1f);
	v3AmbientColor *= m_fAmbientIntensity;
	CUR_SCENE->SetGlobalAmbient(Vector4(v3AmbientColor.x, v3AmbientColor.y, v3AmbientColor.z, 1.0f));
}

float Skybox::NormalizeTimeOfDayHours(float fHours)
{
	if (!std::isfinite(fHours)) {
		return 12.0f;
	}

	float fNormalized = std::fmod(fHours, 24.0f);
	if (fNormalized < 0.0f) {
		fNormalized += 24.0f;
	}

	return fNormalized;
}

float Skybox::GetEffectiveTwilightWidth(float fWidth)
{
	return std::clamp(fWidth, 0.001f, 0.35f);
}

SkyboxData Skybox::MakeCBData() const
{
	return SkyboxData{
		.fDayNightBlend = m_fDayNightBlend,
		.v3SunDirection = m_v3SunDirection,
		.fSunIntensity = m_fSunIntensity,
		.fMoonIntensity = m_fMoonIntensity,
		.fSunDiskSize = m_fSunDiskSize,
		.fMoonDiskSize = m_fMoonDiskSize,
		.fSunGlowSize = m_fSunGlowSize,
		.fMoonGlowSize = m_fMoonGlowSize,
		.fTwilightWidth = GetEffectiveTwilightWidth(m_fTwilightWidth),
		.fTwilightIntensity = m_fTwilightIntensity,
		.fTwilightSunFocus = m_fTwilightSunFocus,
		.fCloudCoverage = m_fCloudCoverage,
		.fCloudDensity = m_fCloudDensity,
		.fCloudSpeed = m_fCloudSpeed,
		.fCloudScale = m_fCloudScale,
		.fCloudLightIntensity = m_fCloudLightIntensity,
		.fStarDensity = m_fStarDensity,
		.fStarScale = m_fStarScale,
		.fSkyIntensity = m_fSkyIntensity,
		.v3TwilightColor = m_v3TwilightColor,
		.v3SunColor = m_v3SunColor,
		.v3MoonColor = m_v3MoonColor,
		.v3DayZenithColor = m_v3DayZenithColor,
		.v3DayHorizonColor = m_v3DayHorizonColor,
		.v3NightZenithColor = m_v3NightZenithColor,
		.v3NightHorizonColor = m_v3NightHorizonColor
	};
}

void Skybox::ShowControllImGui()
{
	if (ImGui::Button("Save parameters")) {
		SaveParametersToJson();
	}

	if (ImGui::DragFloat("Time Of Day (hours)", (float*)&m_fTimeOfDayHours, 0.05f, 0.f, 24.f, "%.2f")) {
		m_fTimeOfDayHours = NormalizeTimeOfDayHours(m_fTimeOfDayHours);
	}
	ImGui::Text("fDayNightBlend : %.3f", m_fDayNightBlend);
	ImGui::DragFloat3("v3NoonHorizontalDirection", (float*)&m_v3NoonHorizontalDirection, 0.01f, -1.f, 1.f);
	ImGui::Text("v3SunDirection : { %f, %f %f }", m_v3SunDirection.x, m_v3SunDirection.y, m_v3SunDirection.z);

	ImGui::NewLine();
	ImGui::DragFloat("fSunIntensity", (float*)&m_fSunIntensity, 0.1f, 0.f, 100.f);
	ImGui::DragFloat("fMoonIntensity", (float*)&m_fMoonIntensity, 0.1f, 0.f, 100.f);
	ImGui::DragFloat("DirectionalLightIntensityScale", (float*)&m_fDirectionalLightIntensityScale, 0.001f, 0.f, 1.f, "%.4f");
	ImGui::DragFloat("AmbientIntensity", (float*)&m_fAmbientIntensity, 0.001f, 0.f, 1.f, "%.4f");
	ImGui::DragFloat("fSunDiskSize", (float*)&m_fSunDiskSize, 0.000001f, 0.99f, 0.99999f, "%.7f");
	ImGui::DragFloat("fMoonDiskSize", (float*)&m_fMoonDiskSize, 0.000001f, 0.99f, 0.99999f, "%.7f");
	ImGui::DragFloat("SunGlowSize", (float*)&m_fSunGlowSize, 0.000001f, 0.99f, 0.99999f, "%.7f");
	ImGui::DragFloat("MoonGlowSize", (float*)&m_fMoonGlowSize, 0.000001f, 0.99f, 0.99999f, "%.7f");

	ImGui::NewLine();
	ImGui::DragFloat("fTwilightWidth", (float*)&m_fTwilightWidth, 0.001f, 0.001f, 0.35f, "%.3f");
	ImGui::DragFloat("fTwilightSunFocus", (float*)&m_fTwilightSunFocus, 0.01f, 0.f, 100.f);
	ImGui::DragFloat("fTwilightIntensity", (float*)&m_fTwilightIntensity, 0.01f, 0.f, 10.f);
	ImGui::DragFloat3("v3TwilightColor", (float*)&m_v3TwilightColor, 0.01f, 0.f, 1.f);

	ImGui::NewLine();
	ImGui::DragFloat("fCloudCoverage", (float*)&m_fCloudCoverage, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("fCloudDensity", (float*)&m_fCloudDensity, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("fCloudSpeed", (float*)&m_fCloudSpeed, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("fSkyIntensity", (float*)&m_fSkyIntensity, 0.01f, 0.f, 1.f);

	ImGui::DragFloat("fCloudScale", (float*)&m_fCloudScale, 0.01f, 0.f, 5.f);
	ImGui::DragFloat("fCloudLightIntensity", (float*)&m_fCloudLightIntensity, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("fStarDensity", (float*)&m_fStarDensity, 0.01f, 0.f, 1.f);
	ImGui::DragFloat("fStarScale", (float*)&m_fStarScale, 0.01f, 0.f, 1.f);

	ImGui::NewLine();
	ImGui::SliderFloat3("v3SunColor", (float*)&m_v3SunColor, 0.f, 1.f);
	ImGui::SliderFloat3("v3MoonColor", (float*)&m_v3MoonColor, 0.f, 1.f); 
	ImGui::SliderFloat3("v3DayZenithColor", (float*)&m_v3DayZenithColor, 0.f, 1.f); 
	ImGui::SliderFloat3("v3DayHorizonColor", (float*)&m_v3DayHorizonColor, 0.f, 1.f); 
	ImGui::SliderFloat3("v3NightZenithColor", (float*)&m_v3NightZenithColor, 0.f, 1.f); 
	ImGui::SliderFloat3("v3NightHorizonColor", (float*)&m_v3NightHorizonColor, 0.f, 1.f); 

}

void Skybox::SaveParametersToJson() const
{
	using namespace nlohmann;

	json j;
	j["v3NoonHorizontalDirection"]	= { m_v3NoonHorizontalDirection.x, m_v3NoonHorizontalDirection.y, m_v3NoonHorizontalDirection.z };
	j["fDayNightBlend"]				= m_fDayNightBlend;
	j["fTimeOfDayHours"]			= m_fTimeOfDayHours;

	j["fSunIntensity"]				= m_fSunIntensity;
	j["fMoonIntensity"]				= m_fMoonIntensity;
	j["fDirectionalLightIntensityScale"] = m_fDirectionalLightIntensityScale;
	j["fAmbientIntensity"]			= m_fAmbientIntensity;
	j["fSunDiskSize"]				= m_fSunDiskSize;
	j["fMoonDiskSize"]				= m_fMoonDiskSize;

	j["fSunGlowSize"]				= m_fSunGlowSize;
	j["fMoonGlowSize"]				= m_fMoonGlowSize;
	j["fTwilightWidth"]				= GetEffectiveTwilightWidth(m_fTwilightWidth);
	j["fTwilightIntensity"]			= m_fTwilightIntensity;

	j["fTwilightSunFocus"]			= m_fTwilightSunFocus;
	j["fCloudCoverage"]				= m_fCloudCoverage;
	j["fCloudDensity"]				= m_fCloudDensity;
	j["fCloudSpeed"]				= m_fCloudSpeed;

	j["fCloudScale"]				= m_fCloudScale;
	j["fCloudLightIntensity"]		= m_fCloudLightIntensity;
	j["fStarDensity"]				= m_fStarDensity;
	j["fStarScale"]					= m_fStarScale;

	j["fSkyIntensity"]				= m_fSkyIntensity;
	j["v3TwilightColor"]			= { m_v3TwilightColor.x, m_v3TwilightColor.y, m_v3TwilightColor.z};
	j["v3SunColor"]					= { m_v3SunColor.x, m_v3SunColor.y, m_v3SunColor.z};
	j["v3MoonColor"]				= { m_v3MoonColor.x, m_v3MoonColor.y, m_v3MoonColor.z};
	j["v3DayZenithColor"]			= { m_v3DayZenithColor.x, m_v3DayZenithColor.y, m_v3DayZenithColor.z};
	j["v3DayHorizonColor"]			= { m_v3DayHorizonColor.x, m_v3DayHorizonColor.y, m_v3DayHorizonColor.z};
	j["v3NightZenithColor"]			= { m_v3NightZenithColor.x, m_v3NightZenithColor.y, m_v3NightZenithColor.z};
	j["v3NightHorizonColor"]		= { m_v3NightHorizonColor.x, m_v3NightHorizonColor.y, m_v3NightHorizonColor.z };

	std::string strSavePath = std::format("{}\\SkyboxParameters.bin", g_strSkyboxBasePath);
	std::ofstream out{ strSavePath, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(j);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());
}
