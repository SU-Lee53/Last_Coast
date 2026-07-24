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
	nointerpolation uint nComponentIndex : TEXCOORD1;
};

struct VS_QUAD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
};

struct VS_QUAD_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

// ================================================================================
// Common quad VS
// ================================================================================

VS_QUAD_OUTPUT VSQuad(VS_QUAD_INPUT input)
{
	VS_QUAD_OUTPUT output = (VS_QUAD_OUTPUT) 0;
	
	output.position = float4(input.position.xy, 0.f, 1.f);
	output.uv = input.uv;
	
	return output;
}

// ================================================================================
// Constants
// ================================================================================

#define PI 3.141592


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
	matrix	mtxInvView;
	matrix	mtxProjection;
	matrix mtxInvProjection;
	float4 gvCascadeSplits; // x, y, z, w
	float3	v3CameraPosition;
	float	pad0;
};

struct SkyboxData
{
	float fDayNightBlend; // 0 : Night, 1 : Day
	float3 v3SunDirection; // -v3SunDirection = v3MoonDirection

	float fSunIntensity;
	float fMoonIntensity;
	float fSunDiskSize;
	float fMoonDiskSize;

	float fSunGlowSize;
	float fMoonGlowSize;
	float fTwilightWidth;
	float fTwilightIntensity;

	float fTwilightSunFocus;
	float fCloudCoverage;
	float fCloudDensity;
	float fCloudSpeed;
	
	float fCloudScale;
	float fCloudLightIntensity;
	float fStarDensity;
	float fStarScale;

	float fSkyIntensity;
	float3 v3TwilightColor; 
	
	float3 v3SunColor; float _pad0;
	float3 v3MoonColor; float _pad1;
	float3 v3DayZenithColor; float _pad2;
	float3 v3DayHorizonColor; float _pad3;
	float3 v3NightZenithColor; float _pad4;
	float3 v3NightHorizonColor; float _pad5;
};

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3
#define MAX_LIGHTS			16

struct LightData
{
	//float4	cAmbient;					// c0
	//float4	cDiffuse;					// c1
	//float4	cSpecular;					// c2
	
	float3	vColor;						// c0.rgb
	float	fIntensity;					// c0.a
	
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
	SkyboxData gSkybox;
	int2 gnScreenSize;
	uint gnToneMappingMode;
};

#define NUM_CASCADES 4
cbuffer cbCascadeShadowMatrix : register(b1, space0)
{
	float4x4 gmtxCascadeShadows[NUM_CASCADES];
}

cbuffer cbShadowMatrix : register(b2, space0)
{
	float4x4 gmtxShadows[4];
}

cbuffer cbFogData : register(b3, space0)
{
	float4 gfogColor;
	
	float gfFogStartDistance;
	float gfFogCutOffDistance;
	float gfFogDistanceDensity;
	float gFogDistancePower;

	float gfFogHeightDensity;
	float gfFogHeightFalloff;
	float gfFogBaseHeight;
	float gfFogHeightStartDistance;

	float gfFogMaxOpacity;
	float3 _pad0;
};

cbuffer cbToneMappingData : register(b4, space0)
{
	float	gfExposure;
	float	gfTargetLuminance;
	float	gfMinExposure;
	float	gfMaxExposure;
	
	int		gnEnableAutoExposure;
	float	gfGamma;
	float	gfPostSaturation;
	float	gfInputScale;
	
	float	gfOutputScale;
	float	gfLookStrength;
	float2	_pad1;
};

cbuffer cbScreenFXData : register(b5, space0)
{
	float gfGrainStrength;
	float gfGrainScale;
	float gfVignetteStrength;
	float gfVignetteRadius;

	float gfVignetteSoftness;
	float3 _pad3;
};

cbuffer cbSSAOData : register(b6, space0)
{
	float gfSSAORadius;
	float gfSSAOBias;
	float gfSSAOPower;
	float gfSSAOIntensity;

	int gnSSAOSampleCount;
	float gfSSAODepthSigma;
	float gfSSAONormalSigma;
	float gfSSAONoiseScale;
};

cbuffer cbLightShaftData : register(b7, space0)
{
	float2 gv2LightScreenPosition;
	float gfLightShaftIntensity;
	float gfLightShaftDecay;

	float gfLightShaftDensity;
	float gfLightShaftWeight;
	float gfLightShaftExposure;
	float gfLightShaftDepthThreshold;

	int gnLightShaftSampleCount;
	int gnEnableLightShaft;
	float2 gLightShaftPad;
};

// ============ StructuredBuffers ============

StructuredBuffer<LightData> gLightData : register(t0, space0);

// ============ Textures ============
Texture2D gtxtCascadeShadowMaps[NUM_CASCADES] : register(t1, space0);	// t1, t2, t3, t4
Texture2D gtxtShadowss[4] : register(t5, space0);						// t5, t6, t7, t8

Texture2D gtxtGBuffer[3] : register(t9, space0);	// t9, t10, t11
Texture2D gtxtGBufferDepth : register(t12, space0);
Texture2D gtxtHDRResult : register(t13, space0);
Texture2D gtxtBloomResult[3] : register(t14, space0);	// t14, t15, t16

Texture2D<float4> gtxtSSAONoise : register(t17, space0);
Texture2D<float> gtxtSSAOInput : register(t18, space0);
Texture2D<float> gtxtSSAOBlur : register(t19, space0);
Texture2D<float> gtxtLuminance : register(t20, space0);

// Tone mapping LUT
Texture3D gtxtToneMapLUT : register(t21, space0);
Texture3D gtxtGradingLUT : register(t22, space0);
Texture2D gtxtLightShaft : register(t23, space0);

// ============ Samplers ============
SamplerState gSkyboxSamplerState : register(s0, space0);
SamplerState gWeightMapSamplerState : register(s1, space0);
SamplerState gSamplerState : register(s2, space0);
SamplerState gPointWrapSamplerState : register(s3, space0);
SamplerState gAnisotropicSamplerState : register(s4, space0);
SamplerComparisonState gShadowMapSamplerState : register(s5, space0);



// ================================================================================
// Per Pass
// ================================================================================

// ============ Structs ============

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASKED 1
#define ALPHA_MODE_TRANSPARENT 2

const static float gfAlphaMaskCutoff = 0.5f;

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
	
	uint eAlphaMode;	// c5.y
	float2 pad0; // c5.zw
};

struct WorldTransformData
{
	float4x4 mtxWorld;
	float4x4 mtxInvWorld;
};

// ============ StructuredBuffers ============

StructuredBuffer<WorldTransformData> gWorldTransforms : register(t0, space1);
StructuredBuffer<matrix> gBoneTransforms : register(t1, space1);
StructuredBuffer<MaterialData> gMaterialDatas : register(t2, space1);

// ============ Textures ============

Texture2D gtxtTextures[] : register(t3, space1); // Unbounded



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
	uint nWeightMapIndex;
	int2 pad0;
};

struct UIRectData
{
	float4 v4ScreenRect;	// left top right bottom
	float4 v4UVRect;
	float4 v4TextColorOrTexIndex;	// Text -> float4 color / Sprite -> float4.x texIndex
	int nTexIndex;
	float fRadialProgress;	// 라디얼 게이지 비율 (0~1). 음수 = 비활성. C++ UIRectData와 레이아웃 일치 필수
};

struct ParticleDrawData
{
	float3 v3Position;
	float fSize;
	
	float4 v4Color;
	float4 v4UVRect;
	
	float fRotation;
	int nTextureIndex;
	float fSoftParticleFactor;
	float pad;
};

#define MAX_BONES 100
#define MAX_TERRAIN_COMPONENTS 64

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
	int gnWorldTransformOffset;	// gWorldTransforms[gnWorldTransformOffset + nInstanceID]
};

#define MAX_LAYER 4

cbuffer cbTerrainLayerData : register(b1, space2)
{
	float4 gv4LayerTiling;
	int gnTerrainLayers;
	float3 pad2;
};

cbuffer cbLightCameraData : register(b3, space2)
{
	matrix gmtxLightViewProj;
}

cbuffer cbTerrainWorldData : register(b4, space2)
{
	matrix gmtxTerrainWorld;
	float3 gv3TerrainScale;
	float gfTerrainWorldPad0;
};

// ============ StructuredBuffers ============

StructuredBuffer<int> gBoneTransformOffsets : register(t2, space2);
Texture2D gtxtTerrainAlbedo[4] : register(t3, space2); // t3, t4, t5, t6
Texture2D gtxtTerrainNormal[4] : register(t7, space2); // t7, t8, t9, t10

// UI
StructuredBuffer<UIRectData> gUIData : register(t12, space2);
StructuredBuffer<ParticleDrawData> gParticleData : register(t13, space2);

// Terrain
StructuredBuffer<TerrainComponentData> gTerrainComponentData : register(t14, space2);
Texture2D<uint> gtxtTerrainHeightMap : register(t15, space2);
Texture2D gtxtTerrainWeightMaps[MAX_TERRAIN_COMPONENTS] : register(t16, space2);

// ================================================================================
// Functions / Helpers
// ================================================================================

#define FLT_EPSILON 1e-8f

float3 BlendTerrainNormal(float2 localXZ, float weights[MAX_LAYER], float3 normalW, float3 tangentW)
{
	float3 bitangentW = normalize(cross(normalW, tangentW));

	float3 blended = float3(0, 0, 0);

    [unroll(MAX_LAYER)]
	for (int layer = 0; layer < gnTerrainLayers; ++layer)
	{
		float w = weights[layer];
		if (w <= 1e-6f)
			continue;

		float2 uv = localXZ * gv4LayerTiling[layer];
		float3 nTS = gtxtTerrainNormal[layer].Sample(gAnisotropicSamplerState, uv).xyz * 2 - 1;

        // TBN
		float3 nW =
            nTS.x * tangentW +
            nTS.y * bitangentW +
            nTS.z * normalW;

		blended += nW * w;
	}

	return normalize(blended);
}

float LoadTerrainHeightLocal(int2 heightCoord)
{
	uint unWidth = 0;
	uint unHeight = 0;
	gtxtTerrainHeightMap.GetDimensions(unWidth, unHeight);
	
	heightCoord = clamp(heightCoord, int2(0, 0), int2((int)unWidth - 1, (int)unHeight - 1));
	uint raw = gtxtTerrainHeightMap.Load(int3(heightCoord, 0));
	return ((int)raw - 32768) * (gv3TerrainScale.y / 128.0f);
}

float4 BlendTerrainAlbedo(float2 localXZ, uint nComponentIndex, out float weights[MAX_LAYER])
{
	[unroll(MAX_LAYER)]
	for (int i = 0; i < MAX_LAYER; ++i)
	{
		weights[i] = 0.0f;
	}
	
	TerrainComponentData componentData = gTerrainComponentData[nComponentIndex];
	float2 vWeightUV = (localXZ - componentData.v2ComponentOriginXZ) / componentData.v2ComponentSizeXZ;
	vWeightUV = saturate(vWeightUV);
	
	// Half tiling
	// 안맞추면 경계면 이상함
	// 조건 : gvNumQuadsXZ + 1.0f 가 WeightMap의 해상도와 일치해야 함
	float2 vWeightMapSize = float2(componentData.v2NumQuadsXZ) + 1.0f;
	vWeightUV += 0.5f / vWeightMapSize;
	
	float4 vWeight = gtxtTerrainWeightMaps[componentData.nWeightMapIndex].Sample(gWeightMapSamplerState, vWeightUV);
	
	// Layer Remapping
	[unroll(MAX_LAYER)]
	for (int channel = 0; channel < MAX_LAYER; ++channel)
	{
		int nLayer = componentData.i4LayerIndex[channel];
		if (nLayer >= 0)
		{
			weights[nLayer] += vWeight[channel];
		}
	}
	
	// Albedo sample + blend
	float4 cFinalColor = 0;
	[unroll(MAX_LAYER)]
	for (int layer = 0; layer < gnTerrainLayers; ++layer)
	{
		float fWeight = weights[layer];
		if (fWeight > 1e-6f)
		{
			float2 vTileUV = localXZ * gv4LayerTiling[layer];
			float4 cAlbedo = gtxtTerrainAlbedo[layer].Sample(gAnisotropicSamplerState, vTileUV);
			cFinalColor += cAlbedo * fWeight;
		}
	}
	
	float fSum = vWeight.r + vWeight.g + vWeight.b + vWeight.a;
	if (fSum > 1e-6f)
	{
		cFinalColor /= fSum;
	}
	
	return cFinalColor;
}

// Octahedral encoding
float2 EncodeNormalOcta(float3 n)
{
	float3 vNormal = normalize(n);
	vNormal /= (abs(vNormal.x) + abs(vNormal.y) + abs(vNormal.z) + FLT_EPSILON);
	
	float2 vEncoded = vNormal.xy;
	if (vNormal.z < 0.f)
	{
		vEncoded = (1.f - abs(vEncoded.yx)) * sign(vEncoded.xy);
	}
	
	vEncoded = vEncoded * 0.5f + 0.5f;	// [-1, 1] -> [0, 1]
	return vEncoded;
}

float3 DecodeNormalOcta(float2 enc)
{
	float2 f = enc * 2.0f - 1.0f;
	float3 n = float3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
	
	float t = saturate(-n.z);
	//n.xy += (n.xy > 0.f) ? -t : t;
	n.x += (n.x >= 0.0f) ? -t : t;
	n.y += (n.y >= 0.0f) ? -t : t;
	
	return normalize(n);
}

void GetMaterialParams(out float fMetallic, out float fRoughness, out float fAO)
{
	MaterialData m = gMaterialDatas[gnMaterialIndex];
	
	fMetallic = m.fMetallic;
	fRoughness = 1.0f - saturate(m.fSmoothness);
	fAO = 1.0f;
}

float3 ComputeNormal(float3 normalW, float3 tangentW, float2 uv)
{
	float3 N = normalize(normalW);
	float3 T = normalize(tangentW - dot(tangentW, N) * N);
	float3 B = -cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
    
	float3 normalFromMap = gtxtTextures[gnTextureIndex.y].Sample(gSamplerState, uv).rgb;
	float3 normal = (normalFromMap * 2.0f) - 1.0f; // [0, 1] -> [-1, 1]
    
	return normalize(mul(normal, TBN));
}

struct GBufferData
{
	float3 albedo;		// RT0.rgb
	float metallic;		// RT0.a

	float3 normalW;		// RT1.rg
	float roughness;	// RT1.b
	float ao;			// RT1.a
	
	float3 emissive;	// RT2.rgb
	float specular;		// RT2.a
	
	float depth;		// depth SRV
};

GBufferData LoadGBuffer(int2 pixel)
{
	GBufferData g;

	float4 rt0 = gtxtGBuffer[0].Load(int3(pixel, 0));
	float4 rt1 = gtxtGBuffer[1].Load(int3(pixel, 0));
	float4 rt2 = gtxtGBuffer[2].Load(int3(pixel, 0));
	float d = gtxtGBufferDepth.Load(int3(pixel, 0)).r;

	g.albedo = rt0.rgb;
	g.metallic = rt0.a;

	g.normalW = DecodeNormalOcta(rt1.rg);
	g.roughness = rt1.b;
	g.ao = rt1.a;

	g.emissive = rt2.rgb;
	g.specular = rt2.a;

	g.depth = d;
	return g;
}

float3 ReconstructViewPos(float2 uv, float depth)
{
	float4 clip;
	clip.x = uv.x * 2.0f - 1.0f;
	clip.y = (1.0f - uv.y) * 2.0f - 1.0f;
	clip.z = depth;
	clip.w = 1.0f;

	float4 view = mul(clip, gCamera.mtxInvProjection);
	return view.xyz / view.w;
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
	float4 clip;
	clip.x = uv.x * 2.0f - 1.0f;
	clip.y = (1.0f - uv.y) * 2.0f - 1.0f;
	clip.z = depth;
	clip.w = 1.0f;

	float4 view = mul(clip, gCamera.mtxInvProjection);
	view /= view.w;

	float4 world = mul(view, gCamera.mtxInvView);
	return world.xyz;
}

float ComputeViewDepth(float3 worldPos)
{
	float3 viewPos = mul(float4(worldPos, 1.f), gCamera.mtxView).xyz;
	return max(0.f, viewPos.z);
}

float ComputeHeightFogOpticalDepth(float3 worldPos)
{
	float3 ray = worldPos - gCamera.v3CameraPosition;
	float fDistToPixel = length(ray);
	
	if (fDistToPixel <= 1e-4f)
	{
		return 0.f;
	}
	
	float3 rayDir = ray / fDistToPixel;
	float fRayLength = max(0.f, fDistToPixel - gfFogHeightStartDistance);
	
	if (gfFogCutOffDistance > 0.f)
	{
		fRayLength = min(fRayLength, gfFogCutOffDistance);
	}
	
	if (fRayLength <= 1e-4f)
	{
		return 0.f;
	}
	
	float3 startPos = gCamera.v3CameraPosition + rayDir * gfFogHeightStartDistance;
	
	float fFallOff = max(1e-4f, gfFogHeightFalloff);
	
	float fStartTerm = exp(-fFallOff * (startPos.y - gfFogBaseHeight));
	float fEndTerm = exp(-fFallOff * (startPos.y + rayDir.y * fRayLength - gfFogBaseHeight));
	
	float fIntegral = (abs(rayDir.y) > 1e-4f) ? (fStartTerm - fEndTerm) / (fFallOff * rayDir.y)
                                              : (fStartTerm * fRayLength);
	
	return gfFogHeightDensity * max(0.f, fIntegral);
}

float ComputeDistanceFogFactor(float3 worldPos)
{
	float fViewDepth = ComputeViewDepth(worldPos);
	float t = saturate((fViewDepth - gfFogStartDistance) / max(1e-4f, gfFogCutOffDistance - gfFogStartDistance));
	t = t * t * (3.0f - 2.0f * t);
	return t;
}

float ComputeHeightFogFactor(float3 worldPos)
{
	float odHeight = ComputeHeightFogOpticalDepth(worldPos);
	return 1.0f - exp(-odHeight);

}

float ComputeFogAlpha(float3 worldPos)
{
	float fDistanceFog = ComputeDistanceFogFactor(worldPos);
	float fHeightFog = ComputeHeightFogFactor(worldPos);
	
	float fFogAlpha = 1.0f - (1.0f - fDistanceFog) * (1.0f - fHeightFog);
	return min(fFogAlpha, gfFogMaxOpacity);
}

float3 ApplyFog(float3 sceneColor, float3 worldPos)
{
	float fFogAlpha = ComputeFogAlpha(worldPos);
	return lerp(sceneColor, gfogColor.rgb, fFogAlpha);
}

#endif 
