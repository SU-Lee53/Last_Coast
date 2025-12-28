#include "pch.h"
#include "Light.h"

LIGHT_DATA PointLight::MakeLightData()
{
	LIGHT_DATA data{};
	{
		data.nType = LIGHT_TYPE_POINT_LIGHT;
		data.bEnable = TRUE;
		data.v4Diffuse = m_v4Diffuse;
		data.v4Ambient = m_v4Ambient;
		data.v4Specular = m_v4Specular;
		data.v3Position = m_v3Position;
		data.fRange = m_fRange;
		data.v3Attenuation.x = m_fAttenuation0;
		data.v3Attenuation.y = m_fAttenuation1;
		data.v3Attenuation.z = m_fAttenuation2;
	}

	return data;
}

LIGHT_DATA SpotLight::MakeLightData()
{
	LIGHT_DATA data{};
	{
		data.nType = LIGHT_TYPE_SPOT_LIGHT;
		data.bEnable = TRUE;
		data.v4Diffuse = m_v4Diffuse;
		data.v4Ambient = m_v4Ambient;
		data.v4Specular = m_v4Specular;
		data.v3Position = m_v3Position;
		data.v3Direction = m_v3Direction;
		data.fRange = m_fRange;
		data.fFalloff = m_fFalloff;
		data.v3Attenuation.x = m_fAttenuation0;
		data.v3Attenuation.y = m_fAttenuation1;
		data.v3Attenuation.z = m_fAttenuation2;
		data.fTheta = m_fTheta;
		data.fPhi = m_fPhi;
	}

	return data;
}

LIGHT_DATA DirectionalLight::MakeLightData()
{
	LIGHT_DATA data{};
	{
		data.nType = LIGHT_TYPE_DIRECTIONAL_LIGHT;
		data.bEnable = TRUE;
		data.v4Diffuse = m_v4Diffuse;
		data.v4Ambient = m_v4Ambient;
		data.v4Specular = m_v4Specular;
		data.v3Direction = m_v3Direction;
	}

	return data;
}
