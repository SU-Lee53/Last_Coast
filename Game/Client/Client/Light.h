#pragma once

#define MAX_LIGHTS			16 

struct LIGHT_DATA {
    Vector4 v4Ambient;                // c0
    Vector4 v4Diffuse;                // c1
    Vector4 v4Specular;               // c2
    Vector3 v3Position;               // c3.xyz
    float fFalloff;                   // c3.w
    Vector3 v3Direction;              // c4.xyz
    float fTheta; //cos(fTheta)     // c4.w
    Vector3 v3Attenuation;            // c5.xyz
    float fPhi; //cos(fPhi)         // c5.w
    BOOL bEnable;                     // c6.x
    int nType;                        // c6.y
    float fRange;                     // c6.z
    float padding;                      // c6.w
};

struct CB_LIGHT_DATA
{
    LIGHT_DATA gLights[MAX_LIGHTS];
    Vector4 gcGlobalAmbientLight;
    int gnLights;
};

enum LIGHT_TYPE : int {
    LIGHT_TYPE_POINT_LIGHT = 1,
    LIGHT_TYPE_SPOT_LIGHT = 2,
    LIGHT_TYPE_DIRECTIONAL_LIGHT = 3,
};

class Light {
public:
    virtual LIGHT_DATA MakeLightData() { return LIGHT_DATA{}; }

public:
    bool m_bEnable;
};

class PointLight : public Light {
public:
    virtual LIGHT_DATA MakeLightData() override;

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
    virtual LIGHT_DATA MakeLightData() override;

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
    virtual LIGHT_DATA MakeLightData() override;

    Vector4    m_v4Diffuse;
    Vector4    m_v4Ambient;
    Vector4    m_v4Specular;

    Vector3    m_v3Direction;
};

