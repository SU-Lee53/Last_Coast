#pragma once

enum class LIGHT_TYPE : int32 {
    POINT_LIGHT = 1,
    SPOT_LIGHT = 2,
    DIRECTIONAL_LIGHT = 3,
};

class Light {
public:
    virtual LightData MakeCBData() { return LightData{}; }

	virtual void ShowControllImGui() { };

public:
	Vector3		m_v3Position;
	float       m_fRange;

    bool m_bEnable = true;
};

class PointLight : public Light {
public:
    virtual LightData MakeCBData() override;
	virtual void ShowControllImGui() override;

    //Vector4    m_v4Diffuse;
    //Vector4    m_v4Ambient;
    //Vector4    m_v4Specular;

	Vector3		m_v3Color;
	float		m_fIntensity;

    Vector3		m_v3Direction;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;

};

class SpotLight : public Light {
public:
    virtual LightData MakeCBData() override;
	virtual void ShowControllImGui() override;

	Vector3		m_v3Color;
	float		m_fIntensity;

    Vector3		m_v3Direction;

    float       m_fFalloff;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;
    float       m_fTheta;
    float       m_fPhi;
};

class DirectionalLight : public Light {
public:
    virtual LightData MakeCBData() override;
	virtual void ShowControllImGui() override;

	Vector3		m_v3Color;
	float		m_fIntensity;

    Vector3    m_v3Direction;
};

