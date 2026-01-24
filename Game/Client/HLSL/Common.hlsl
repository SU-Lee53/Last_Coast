#ifndef _COMMON_
#define _COMMON_


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shader Inputs

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

struct VS_SKINNED_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
	
    uint4 blendInices : BLENDINDICES;
    float4 blendWeights : BLENDWEIGHTS;
};

struct VS_SKINNED_OUTPUT
{
    float4 position : SV_POSITION;
	float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float2 uv : TEXCOORD0;
};

struct VS_TERRAIN_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VS_TERRAIN_OUTPUT
{
    float4 position : SV_POSITION;
	float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
	
	float2 positionLocalXZ : TEXCOORD;
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

cbuffer cbTerrainLayerData : register(b2)
{
	float4 gvLayerTiling;
	int gnLayers;
}

#define MAX_LAYER 4
Texture2D gtxtTerrainAlbedo[MAX_LAYER] : register(t1); // t1, t2, t3, t4
Texture2D gtxtTerrainNormal[MAX_LAYER] : register(t5); // t5, t6, t7, t8

cbuffer cbTerrainComponentData : register(b3)
{
	float2	gvComponentOriginXZ;
	float2	gvComponentSizeXZ;
	int4	gLayerIndex;
	int2	gvNumQuadsXZ;
}

Texture2D gtxtComponentWeightMap : register(t9);

SamplerState gWeightMapSamplerState : register(s2);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pass
StructuredBuffer<matrix> gsbInstanceDatas : register(t10);

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

cbuffer cbGameObjectInfo : register(b4)
{
    MATERIAL gMaterial;
    int gnInstanceBase;
};

cbuffer cbWorldTransformData : register(b5)
{
    matrix gmtxWorld;
}

#define MAX_BONES 150

cbuffer cbBoneTransformData : register(b6)
{
	matrix boneTransforms[MAX_BONES];
}

Texture2D gtxtDiffuseTexture : register(t11);
Texture2D gtxtNormalTexture : register(t12);
Texture2D gtxtMetaillicTexture : register(t13);
Texture2D gtxtEmissionTexture : register(t14);

SamplerState gSamplerState : register(s1);

#endif
