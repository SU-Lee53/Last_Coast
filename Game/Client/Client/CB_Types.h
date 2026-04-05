#pragma once

constexpr static UINT MAX_BONE_TRANSFORMS = 100;
constexpr static UINT MAX_EFFECT_PER_DRAW = 100;
constexpr static UINT MAX_CHARACTER_PER_SPRITE = 40;
constexpr static UINT MAX_LIGHTS = 16;

//////////////////////////////////////////////////////////////////////////////////
// Per Scene (Frame)

// ============ Structs ============

struct CameraData {
	Matrix	mtxView;
	Matrix	mtxInvView;
	Matrix	mtxProjection;
	Matrix	mtxInvProjection;
	Vector4 v4CascadeSplits;
	Vector3	v3CameraPosition;
	float	pad0;
};

struct SceneGlobalData {
	Vector4		v4GlobalAmbient;
	float		fTotalTime;
	float		fElapsedTime;
	int32		nNumLights;
	float		pad0;
};

struct LightData
{
	//Vector4		v4Ambient;
	//Vector4		v4Diffuse;
	//Vector4		v4Specular;

	Vector3 v3Color;						// c0.xyz
	float fIntensity;						// c0.w
	Vector3		v3Position;					// c1.xyz
	float		fFalloff;					// c1.w
	Vector3		v3Direction;				// c2.xyz
	float		fTheta; //cos(m_fTheta)		// c2.w
	Vector3		v3Attenuation;				// c3.xyz
	float		fPhi; //cos(m_fPhi)			// c4.w
	uint32		bEnable;					// c5.x
	uint32		nType;						// c5.y
	float		fRange;						// c5.z
	float		pad0;						// c5.w
};

struct SkyboxData {
	float fDayNightBlend;	// 0 : Night, 1 : Day
	Vector3 v3SunDirection;	// -v3SunDirection = v3MoonDirection

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

	float	fSkyIntensity;
	Vector3 v3TwilightColor;

	Vector3 v3SunColor;  float _pad0;
	Vector3 v3MoonColor;  float _pad1;
	Vector3 v3DayZenithColor; float _pad2;
	Vector3 v3DayHorizonColor; float _pad3;
	Vector3 v3NightZenithColor; float _pad4;
	Vector3 v3NightHorizonColor; float _pad5;
};

// ============ cbuffers ============

struct alignas(16) CB_SCENE_DATA
{
	CameraData gCamera;
	SceneGlobalData gSceneGlobal;
	SkyboxData gSkybox;
	XMINT2 nScreenSize;
};

struct CB_TO_SHADOW_MATRICES_DATA 
{
	Matrix mtxToShadows[4];
};

//////////////////////////////////////////////////////////////////////////////////
// Global Resources

// ============ Structs ============

struct MaterialData
{
	Vector4 v4Ambient;						// c0
	Vector4 v4Diffuse;						// c1
	Vector4 v4Specular; //(r,g,b,a=power)	// c2
	Vector4 v4Emissive;						// c3

	float fGlossiness;						// c4.x
	float fSmoothness;						// c4.y
	float fSpecularHighlight;				// c4.z
	float fMetallic;						// c4.w
	float fGlossyReflection;				// c5.x

	uint32 eAlphaMode;						// c5.y
	Vector2 pad0;							// c5.zw
};

struct WorldTransformData
{
	Matrix mtxWorld;
	Matrix mtxInvWorld;
};

//////////////////////////////////////////////////////////////////////////////////
// Per Instance (Object)

// ============ Structs ============

// ============ cbuffers ============

struct CB_INSTANCE_DATA
{
	int gnTextureIndex[4] = {-1, -1, -1, -1};	// Diffuse, Normal, Metallic, Emission
	int gnMaterialIndex;
	int gnWorldTransformOffset;
};

struct CB_WORLD_BASE_DATA
{
	int gnInstanceDataBase;
};

struct CB_TERRAIN_LAYER_DATA
{
	Vector4 v4LayerTiling;
	int nLayers;
	Vector3 pad0;
};

struct CB_TERRAIN_COMPONENT_DATA
{
	Vector2 v2ComponentOriginXZ;
	Vector2 v2ComponentSizeXZ;
	XMINT4 xmi4LayerIndex;
	XMINT2 xmi2NumQuadsXZ;
	Vector2 pad0;
};




struct CB_PARTICLE_DATA {
	EffectParameter parameters[MAX_EFFECT_PER_DRAW];
};

struct CB_TEXT_DATA {
	UINT nCharacters[MAX_CHARACTER_PER_SPRITE];
	XMFLOAT4 xmf4TextColor;
	UINT nLength;
};

struct CB_BILLBOARD_SPRITE_DATA {
	XMFLOAT3 xmf3Position;
	UINT pad1 = 0;
	XMFLOAT2 xmf2Size;
	XMUINT2 pad2 = XMUINT2(0, 0);
	XMFLOAT3 xmf3CameraPosition;
	UINT pad3 = 0;
	XMFLOAT4X4 xmf4x4ViewProjection;
};
