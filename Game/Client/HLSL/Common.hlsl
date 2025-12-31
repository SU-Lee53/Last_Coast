#ifndef _COMMON_
#define _COMMON_


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS_TEXTURED_NORMAL_TANGENT_INPUT

struct VS_STANDARD_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

struct VS_STANDARD_OUTPUT
{
    float4 position : SV_POSITION;
	float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float2 uv : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene

#define MAX_LIGHTS			16 
#define MAX_MATERIALS		512 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

#define _WITH_LOCAL_VIEWER_HIGHLIGHTING
#define _WITH_THETA_PHI_CONES
//#define _WITH_REFLECT

struct LIGHT
{
    float4      cAmbient;
    float4      cDiffuse;
    float4      cSpecular;
    float3      vPosition;
    float       fFalloff;
    float3      vDirection;
    float       fTheta; //cos(m_fTheta)
    float3      vAttenuation;
    float       fPhi; //cos(m_fPhi)
    bool        bEnable;
    int         nType;
    float       fRange;
    float       padding;
};

cbuffer cbCamera : register(b0)
{
	matrix gmtxView;
	matrix gmtxProjection;
	float3 gvCameraPosition;
}

cbuffer cbLights : register(b1)
{
    LIGHT gLights[MAX_LIGHTS];
    float4 gcGlobalAmbientLight;
    int gnLights;
};

Texture2DArray gtxtSkyboxTextures : register(t0);
SamplerState gSkyboxSamplerState : register(s0);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pass

StructuredBuffer<matrix> gsbInstanceDatas : register(t1);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Object

struct MATERIAL
{
    float4 cAmbient;
    float4 cDiffuse;
    float4 cSpecular; //(r,g,b,a=power)
    float4 cEmissive;

    float fGlossiness;
    float fSmoothness;
    float fSpecularHighlight;
    float fMetallic;
    float fGlossyReflection;
};

cbuffer cbGameObjectInfo : register(b2)
{
    MATERIAL gMaterial;
    int gnInstanceBase;
};

cbuffer cbWorldTransformData : register(b3)
{
    matrix gmtxWorld;
}

Texture2D gtxtDiffuseTexture : register(t2);
Texture2D gtxtNormalTexture : register(t3);
Texture2D gtxtMetaillicTexture : register(t4);
Texture2D gtxtEmissionTexture : register(t5);

SamplerState gSamplerState : register(s1);

#endif
