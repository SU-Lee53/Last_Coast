#ifndef _NEW_COMMON_
#define _NEW_COMMON_

// ================================================================================
//  Shader Inputs
// ================================================================================

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
	
	float2 positionLocalXZ : TEXCOORD0;
};

// ================================================================================
// Per Scene (Frame)
// ================================================================================

// ============ Structs ============

struct SceneGlobalData
{
	float4	v4GlobalAmbient;
	float	fTotalTime;
	float	fElapsedTime;
	int		nNumLights;
	float	pad0;
};

struct CameraData
{
	matrix	mtxView;
	matrix	mtxProjection;
	float3	v3CameraPosition;
	float	pad0;
};

struct LightData
{
	float4	cAmbient;					// c0
	float4	cDiffuse;					// c1
	float4	cSpecular;					// c2
	float3	vPosition;					// c3.xyz
	float	fFalloff;					// c3.w
	float3	vDirection;					// c4.xyz
	float	fTheta; //cos(m_fTheta)		// c4.w
	float3	vAttenuation;				// c5.xyz
	float	fPhi; //cos(m_fPhi)			// c6.w
	uint	bEnable;					// c7.x
	int		nType;						// c7.y
	float	fRange;						// c7.z
	float	pad0;						// c7.w
};

// ============ cbuffers ============

cbuffer cbSceneData : register(b0, space0)
{
	CameraData gCamera;
	SceneGlobalData gSceneGlobal;
};

// ============ StructuredBuffers ============

StructuredBuffer<LightData> gLightData : register(t0, space0);

// ============ Textures ============
Texture2DArray gtxtSkyboxDay : register(t1, space0);
Texture2DArray gtxtSkyboxNIght : register(t2, space0);

Texture2D gtxtShadows[8] : register(t11, space0);	// t11, t12, t13, t14, t15, t16, t17, t18

// ============ Samplers ============
SamplerState gSkyboxSamplerState : register(s0, space0);
SamplerState gWeightMapSamplerState : register(s1, space0);
SamplerState gSamplerState : register(s2, space0);



// ================================================================================
// Per Pass
// ================================================================================

// ============ Structs ============

struct MaterialData
{
	float4 cAmbient; // c0
	float4 cDiffuse; // c1
	float4 cSpecular; //(r,g,b,a=power)		// c2
	float4 cEmissive; // c3
	
	float fGlossiness; // c4.x
	float fSmoothness; // c4.y
	float fSpecularHighlight; // c4.z
	float fMetallic; // c4.w
	float fGlossyReflection; // c5.x
	
	float3 pad0; // c6.yzw
};

// ============ StructuredBuffers ============

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

// ============ Textures ============

Texture2D gtxtTextures[] : register(t1, space1); // Unbounded



// ================================================================================
// Per Instance (Object)
// ================================================================================

// ============ Structs ============

struct TerrainComponentData
{
	float2 v2ComponentOriginXZ;
	float2 v2ComponentSizeXZ;
	int4 i4LayerIndex;
	int2 v2NumQuadsXZ;
	int2 pad0;
};

struct InstanceData
{
	float4x4 mtxWorld;
};

#define MAX_BONES 100
#define MAX_TERRAIN_COMPONENTS 8*8

// ============ cbuffers ============

#define TEXTURE_TYPE_ALBEDO			0
#define TEXTURE_TYPE_NORMAL			1
#define TEXTURE_TYPE_METALLIC		2
#define TEXTURE_TYPE_EMISSION		3

#define TEXTURE_TYPE_DIFFUSE 0

cbuffer cbInstanceData : register(b0, space2)
{
	int4 gnTextureIndex;	// Diffuse, Normal, Metallic, Emission
	int gnMaterialIndex;
};

#define MAX_LAYER 4

cbuffer cbTerrainLayerData : register(b1, space2)
{
	float4 gv4LayerTiling;
	int gnTerrainLayers;
	float3 pad0;
};

cbuffer cbTerrainComponentData : register(b2, space2)
{
	float2 gv2ComponentOriginXZ;
	float2 gv2ComponentSizeXZ;
	int4 gi4LayerIndex;
	int2 gv2NumQuadsXZ;
	int2 pad1;
};

// ============ StructuredBuffers ============

StructuredBuffer<InstanceData> gWorldTransforms : register(t0, space2);
StructuredBuffer<matrix> gBoneTransforms : register(t1, space2);


Texture2D gtxtTerrainAlbedo[4] : register(t2, space2); // t2, t3, t4, t5
Texture2D gtxtTerrainNormal[4] : register(t6, space2); // t6, t7, t8, t9

Texture2D gtxtTerrainWeightMap : register(t10, space2);


#endif 
