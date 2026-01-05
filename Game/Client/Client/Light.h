#pragma once

enum LIGHT_TYPE : int {
    LIGHT_TYPE_POINT_LIGHT = 1,
    LIGHT_TYPE_SPOT_LIGHT = 2,
    LIGHT_TYPE_DIRECTIONAL_LIGHT = 3,
};

class Light {
public:
    virtual LightData MakeLightData() { return LightData{}; }

public:
    bool m_bEnable;
};

class PointLight : public Light {
public:
    virtual LightData MakeLightData() override;

    Vector4    m_v4Diffuse;
    Vector4    m_v4Ambient;
    Vector4    m_v4Specular;

    Vector3    m_v3Position;
    Vector3    m_v3Direction;

    float       m_fRange;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;

};

class SpotLight : public Light {
public:
    virtual LightData MakeLightData() override;

    Vector4    m_v4Diffuse;
    Vector4    m_v4Ambient;
    Vector4    m_v4Specular;

    Vector3    m_v3Position;
    Vector3    m_v3Direction;

    float       m_fRange;
    float       m_fFalloff;
    float       m_fAttenuation0;
    float       m_fAttenuation1;
    float       m_fAttenuation2;
    float       m_fTheta;
    float       m_fPhi;
};

class DirectionalLight : public Light {
public:
    virtual LightData MakeLightData() override;

    Vector4    m_v4Diffuse;
    Vector4    m_v4Ambient;
    Vector4    m_v4Specular;

    Vector3    m_v3Direction;
};

