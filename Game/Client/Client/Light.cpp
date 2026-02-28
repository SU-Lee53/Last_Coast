#include "pch.h"
#include "Light.h"

LightData PointLight::MakeCBData()
{
	return LightData{
		.v4Ambient = m_v4Ambient,
		.v4Diffuse = m_v4Diffuse,
		.v4Specular = m_v4Specular,
		.v3Position = m_v3Position,
		.v3Attenuation = Vector3{m_fAttenuation0, m_fAttenuation1, m_fAttenuation2},
		.bEnable = TRUE,
		.nType = LIGHT_TYPE_POINT_LIGHT,
		.fRange = m_fRange,
	};
}

LightData SpotLight::MakeCBData()
{
	return LightData{
		.v4Ambient = m_v4Ambient,
		.v4Diffuse = m_v4Diffuse,
		.v4Specular = m_v4Specular,
		.v3Position = m_v3Position,
		.fFalloff = m_fFalloff,
		.v3Direction = m_v3Direction,
		.fTheta = m_fTheta,
		.v3Attenuation = Vector3{m_fAttenuation0, m_fAttenuation1, m_fAttenuation2},
		.fPhi = m_fPhi,
		.bEnable = TRUE,
		.nType = LIGHT_TYPE_SPOT_LIGHT,
		.fRange = m_fRange,
		.pad0 = 0.f
	};
}

LightData DirectionalLight::MakeCBData()
{
	return LightData {
		.v4Ambient = m_v4Ambient,
		.v4Diffuse = m_v4Diffuse,
		.v4Specular = m_v4Specular,
		.v3Direction = m_v3Direction,
		.bEnable = TRUE,
		.nType = LIGHT_TYPE_DIRECTIONAL_LIGHT,
	};
}
