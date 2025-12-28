#ifndef _COMMON_
#define _COMMON_


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DIFFUSED INOUT

struct VS_DIFFUSED_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VS_DIFFUSED_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEXTURED INOUT

struct VS_TEXTURED_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_TEXTURED_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TEXTURED_NORMAL INOUT

struct VS_TEXTURED_NORMAL_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_TEXTURED_NORMAL_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS_TEXTURED_NORMAL_TANGENT_INPUT INOUT

struct VS_TEXTURED_NORMAL_TANGENT_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

struct VS_TEXTURED_NORMAL_TANGENT_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 posW : POSITION;
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

struct CAMERA
{
    matrix gmtxView;
    matrix gmtxProjection;
    float3 gvCameraPosition;
};

cbuffer cbCamera : register(b0)
{
    CAMERA gCameraData;
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
    matrix mtxWorld;
}

Texture2D gtxtDiffuseTexture : register(t2);
Texture2D gtxtNormalTexture : register(t3);

SamplerState gSamplerState : register(s1);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Others

cbuffer cbPlayerData : register(b4)
{
    float3 gvPositionW;
    float pad0 = 0.f;
    float3 gvLookW;             // ray direction
    float pad1 = 0.f;
    float3 gvUpW;               // ray Up -> 축 구성을 위함
    float pad2 = 0.f;
    float gfRayLength;          // ray length
    float gfRayHalfWidth;       // ray halfWidth
    float2 gvBillboardSizeW;    // billboard
    float3 gvRayPositionW;
    float pad3 = 0.f;
    float3 gvRaDirectionW; // ray direction
    float pad4 = 0.f;
};

Texture2D gtxtBillboardTexture : register(t4);  // billboard texture

#endif