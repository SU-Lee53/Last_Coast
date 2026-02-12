#ifndef _COMMON_
#define _COMMON_


//////////////////////////////////////////////////////////////////////////////////
// Per Scene (Frame)

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

struct TerrainLayerData
{
	float4 v4LayerTiling;
	int nLayers;
	float3 pad0;
};

// ============ cbuffers ============

cbuffer cbSceneData : register(b0, space0)
{
	CameraData gCamera;
	SceneGlobalData gSceneGlobal;
	TerrainLayerData gTerrainLayer;
};

// ============ StructuredBuffers ============

StructuredBuffer<LightData> gLightData : register(t0, space0);

// ============ Textures ============
Texture2DArray gtxtSkyboxDay : register(t1, space0);
Texture2DArray gtxtSkyboxNIght : register(t2, space0);

Texture2D gtxtTerrainAlbedo[4] : register(t3, space0);	// t3, t4, t5, t6
Texture2D gtxtTerrainNormal[4] : register(t7, space0); // t7, t8, t9, t10

Texture2D gtxtShadows[8] : register(t11, space0);	// t11, t12, t13, t14, t15, t16, t17, t18

// ============ Samplers ============
SamplerState gSkyboxSamplerState : register(s0, space0);
SamplerState gWeightMapSamplerState : register(s1, space0);
SamplerState gSamplerState : register(s2, space0);


//////////////////////////////////////////////////////////////////////////////////
// Global Resources

// ============ Structs ============

struct MaterialData
{
	float4 cAmbient;						// c0
	float4 cDiffuse;						// c1
	float4 cSpecular; //(r,g,b,a=power)		// c2
	float4 cEmissive;						// c3
	
	float fGlossiness;						// c4.x
	float fSmoothness;						// c4.y
	float fSpecularHighlight;				// c4.z
	float fMetallic;						// c4.w
	float fGlossyReflection;				// c5.x
	
	float3 pad0;							// c6.yzw
};

struct InstanceData
{
	float4x4 mtxWorld;
};

// ============ StructuredBuffers ============

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
StructuredBuffer<InstanceData> gInstanceData : register(t1, space1);
Texture2D gtxtTextures[] : register(t2, space1); // Unbounded

//////////////////////////////////////////////////////////////////////////////////
// Per Instance (Object)

// ============ Structs ============

struct TerrainComponentData
{
	float2 v2ComponentOriginXZ;
	float2 v2ComponentSizeXZ;
	int4 i4LayerIndex;
	int2 v2NumQuadsXZ;
	int2 pad0;
};

#define MAX_BONES 100
#define MAX_TERRAIN_COMPONENTS 8*8

// ============ cbuffers ============

cbuffer cbInstanceData : register(b0, space2)
{
	int gnTextureIndex[4];	// Diffuse, Normal, Metallic, Emission
	int gnInstanceBase;
	int gnMaterialIndex;
	int2 pad0;
};

// ============ StructuredBuffers ============

StructuredBuffer<TerrainComponentData> gTerrainComponents : register(t0, space2);
StructuredBuffer<float4x4> gBoneTransforms : register(t1, space2);


#endif 
