#pragma once


//////////////////////////////////////////////////////////////////////////////////
// Macros

#define DECLARE_SINGLE(classname)			\
private:									\
	classname() { }							\
public:										\
	static classname* GetInstance()			\
	{										\
		static classname s_instance;		\
		return &s_instance;					\
	}										\


#define GET_SINGLE(classname)	classname::GetInstance()

#define RESOURCE		GET_SINGLE(ResourceManager)
#define RENDER			GET_SINGLE(RenderManager)
#define SHADER			GET_SINGLE(ShaderManager)
#define TEXTURE			GET_SINGLE(TextureManager)
#define SCENE			GET_SINGLE(SceneManager)
#define INPUT			GET_SINGLE(InputManager)
#define TIMER			GET_SINGLE(GameTimer)
#define MODEL			GET_SINGLE(ModelManager)
#define GUI				GET_SINGLE(GuiManager)
#define NETWORK			GET_SINGLE(NetworkManager)
#define EFFECT			GET_SINGLE(EffectManager)
#define SOUND			GET_SINGLE(SoundManager)
#define UI				GET_SINGLE(UIManager)
#define TIME			GET_SINGLE(GameTimer)
#define ANIMATION		GET_SINGLE(AnimationManager)

#define CUR_SCENE		SCENE->GetCurrentScene()
#define DT				TIME->GetTimeElapsed()

//////////////////////////////////////////////////////////////////////////////////
// Constants
constexpr static float PLANET_ROTATION = 20.f;


//////////////////////////////////////////////////////////////////////////////////
// Enums

enum SHADER_RESOURCE_TYPE : UINT8 {
	SHADER_RESOURCE_TYPE_CONSTANT_BUFFER = 0,
	SHADER_RESOURCE_TYPE_TEXTURE,
	SHADER_RESOURCE_TYPE_STRUCTURED_BUFFER,

	SHADER_RESOURCE_TYPE_COUNT,

	SHADER_RESOURCE_TYPE_UNDEFINED = 99
};

enum ROOT_PARAMETER_TYPE : UINT8 {
	// Root Constant 는 사용하지 않을듯
	ROOT_PARAMETER_TYPE_ROOT_DESCRIPTOR,
	ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,

	ROOT_PARAMETER_TYPE_COUNT,

	ROOT_PARAMETER_TYPE_UNDEFINED = 99
};

enum COMPONENT_TYPE : UINT8 {
	//COMPONENT_TYPE_MOVEMENT,			// undefined
	//COMPONENT_TYPE_MESH_RENDERER,		// 이제 Component 아님
	//COMPONENT_TYPE_ANIMATION,			// undefined
	//COMPONENT_TYPE_BEHAVIOR_TREE,		// undefined
	//COMPONENT_TYPE_PHYSICS,				// undefined

	COMPONENT_TYPE_COUNT,

	COMPONENT_TYPE_BASE
};

enum MESH_ELEMENT_TYPE : UINT {
	MESH_ELEMENT_TYPE_POSITION = 0x0001,
	MESH_ELEMENT_TYPE_COLOR = 0x0002,
	MESH_ELEMENT_TYPE_NORMAL = 0x0004,
	MESH_ELEMENT_TYPE_TANGENT = 0x0008,

	MESH_ELEMENT_TYPE_TEXCOORD0 = 0x0010,
	MESH_ELEMENT_TYPE_TEXCOORD1 = 0x0020,
	MESH_ELEMENT_TYPE_TEXCOORD2 = 0x0040,
	MESH_ELEMENT_TYPE_TEXCOORD3 = 0x0080,

	MESH_ELEMENT_TYPE_BLEND_INDICES = 0x0100,
	MESH_ELEMENT_TYPE_BLEND_WEIGHTS = 0x0200,

	MESH_ELEMENT_TYPE_END = 0x0400,
};

enum ANIMATION_PLAY_TYPE : UINT {
	ANIMATION_PLAY_ONCE,
	ANIMATION_PLAY_LOOP,
};

enum SPRITE_TYPE : UINT8 {
	SPRITE_TYPE_TEXTURE = 0,
	SPRITE_TYPE_TEXT,
	SPRITE_TYPE_BILLBOARD,

	SPRITE_TYPE_COUNT
};

//////////////////////////////////////////////////////////////////////////////////
// Structs

struct LightData {
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

struct MaterialColors {
	XMFLOAT4		xmf4Ambient;
	XMFLOAT4		xmf4Diffuse;
	XMFLOAT4		xmf4Specular; //(r,g,b,a=power)
	XMFLOAT4		xmf4Emissive;

	float			fGlossiness = 0.0f;
	float			fSmoothness = 0.0f;
	float			fSpecularHighlight = 0.0f;
	float			fMetallic = 0.0f;
	float			fGlossyReflection = 0.0f;
};

struct EffectParameter {
	Vector3		xmf3Position;
	float		fElapsedTime = 0.f;
	Vector3		xmf3Force;
	float		fAdditionalData = 0.f;
};

struct SpriteRect {
	float fLeft;
	float fTop;
	float fRight;
	float fBottom;
};

struct Bone {
	std::string strBoneName;
	int nIndex;
	int nParentIndex;
	Matrix mtxTransform;
	Matrix mtxOffset;

	int nChildren = 0;
	std::vector<int> nChilerenIndex;
	int nDepth = 0;
};

struct PendingUploadBuffer {
	ComPtr<ID3D12Resource> pd3dPendingUploadBuffer = nullptr;
	CommandListPair* cmdListPair;	// Only for ref
};

//////////////////////////////////////////////////////////////////////////////////
// CB Types

constexpr static UINT MAX_BONE_TRANSFORMS		= 150;
constexpr static UINT MAX_EFFECT_PER_DRAW		= 100;
constexpr static UINT MAX_CHARACTER_PER_SPRITE	= 40;
constexpr static UINT MAX_LIGHTS = 16;

struct CB_CAMERA_DATA
{
	Matrix	mtxView;
	Matrix	mtxProjection;
	Vector3 v3CameraPosition;
};

struct CB_PER_OBJECT_DATA {
	MaterialColors materialColors;
	int nInstanceBase;
};

struct CB_WORLD_TRANSFORM_DATA {
	Matrix mtxTransforms;
};

struct CB_BONE_TRANSFORM_DATA
{
	Matrix mtxBoneTransforms[MAX_BONE_TRANSFORMS];
};

struct CB_PARTICLE_DATA {
	EffectParameter parameters[MAX_EFFECT_PER_DRAW];
};

struct CB_LIGHT_DATA
{
	LightData gLights[MAX_LIGHTS];
	Vector4 gcGlobalAmbientLight;
	int gnLights;
};

struct CB_SPRITE_DATA {
	float fLeft;
	float fTop;
	float fRight;
	float fBottom;
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

struct AnimationKey {
	Vector3 v3Translation{ 0.f, 0.f, 0.f };
	Quaternion v4RotationQuat{ 0.f, 0.f, 0.f, 1.f };
	Vector3 v3Scale{ 1.f, 1.f, 1.f };

	Matrix CreateSRT() const {
		Matrix mtxScale = Matrix::CreateScale(v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(v4RotationQuat);
		Matrix mtxTranslation = Matrix::CreateTranslation(v3Translation);
		return mtxScale * mtxRotation * mtxTranslation;
	}

	static Matrix CreateSRT(const Vector3& v3Translation, const Quaternion& v4RotationQuat, const Vector3& v3Scale) {
		Matrix mtxScale = Matrix::CreateScale(v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(v4RotationQuat);
		Matrix mtxTranslation = Matrix::CreateTranslation(v3Translation);
		return mtxScale * mtxRotation * mtxTranslation;
	}

	static AnimationKey Lerp(const AnimationKey& key1, const AnimationKey& key2, float fWeight) {
		Vector3 v3Scale = Vector3::Lerp(key1.v3Scale, key2.v3Scale, fWeight);
		Quaternion v4Rotation = Quaternion::Slerp(key1.v4RotationQuat, key2.v4RotationQuat, fWeight);
		Vector3 v3Translation = Vector3::Lerp(key1.v3Translation, key2.v3Translation, fWeight);

		return { v3Translation, v4Rotation, v3Scale };
	}

	static AnimationKey SmoothStep(const AnimationKey& key1, const AnimationKey& key2, float fWeight) {
		Vector3 v3Scale = Vector3::SmoothStep(key1.v3Scale, key2.v3Scale, fWeight);
		Quaternion v4Rotation = Quaternion::Slerp(key1.v4RotationQuat, key2.v4RotationQuat, fWeight);
		Vector3 v3Translation = Vector3::SmoothStep(key1.v3Translation, key2.v3Translation, fWeight);

		return { v3Translation, v4Rotation, v3Scale };
	}

};

//////////////////////////////////////////////////////////////////////////////////
// Default Constants

constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAG_DEFAULT = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
