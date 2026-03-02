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
	Matrix	mtxProjection;
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
	Vector4		v4Ambient;					// c0
	Vector4		v4Diffuse;					// c1
	Vector4		v4Specular;					// c2
	Vector3		v3Position;					// c3.xyz
	float		fFalloff;					// c3.w
	Vector3		v3Direction;					// c4.xyz
	float		fTheta; //cos(m_fTheta)		// c4.w
	Vector3		v3Attenuation;				// c5.xyz
	float		fPhi; //cos(m_fPhi)			// c6.w
	uint32		bEnable;					// c7.x
	uint32		nType;						// c7.y
	float		fRange;						// c7.z
	float		pad0;						// c7.w
};

// ============ cbuffers ============

struct alignas(16) CB_SCENE_DATA
{
	CameraData gCamera;
	SceneGlobalData gSceneGlobal;
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

	Vector3 pad0;							// c6.yzw
};

struct InstanceData
{
	Matrix mtxWorld;
};

//////////////////////////////////////////////////////////////////////////////////
// Per Instance (Object)

// ============ Structs ============

// ============ cbuffers ============

struct CB_INSTANCE_DATA
{
	int gnTextureIndex[4] = {-1, -1, -1, -1};	// Diffuse, Normal, Metallic, Emission
	int gnMaterialIndex;
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
