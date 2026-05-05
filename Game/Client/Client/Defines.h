#pragma once

class IGameObject;
class ICollider;

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
#define TIME			GET_SINGLE(GameTimer)
#define MODEL			GET_SINGLE(ModelManager)
#define GUI				GET_SINGLE(GuiManager)
#define NETWORK			GET_SINGLE(NetworkManager)
//#define SESSION			GET_SINGLE(ServerSession)
#define PARTICLE		GET_SINGLE(ParticleManager)
#define SOUND			GET_SINGLE(SoundManager)
#define UI				GET_SINGLE(UIManager)
#define ANIMATION		GET_SINGLE(AnimationManager)
#define AI				GET_SINGLE(AIManagerWrapper)
#define MATERIAL		GET_SINGLE(MaterialManager)
#define COMPUTE			GET_SINGLE(ComputeManager)
//#define TEXT			GET_SINGLE(TextManager)

#define CUR_SCENE		SCENE->GetCurrentScene()
#define DT				TIME->GetTimeElapsed()

//////////////////////////////////////////////////////////////////////////////////
// Constants
constexpr static uint64 INVALID_ID = std::numeric_limits<uint64>::max();

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

enum class MESH_ELEMENT_TYPE : uint32 {
	POSITION = 1,
	COLOR = POSITION << 1,
	NORMAL = COLOR << 1,
	TANGENT = NORMAL << 1,

	TEXCOORD0 = TANGENT << 1,
	TEXCOORD1 = TEXCOORD0 << 1,
	TEXCOORD2 = TEXCOORD1 << 1,
	TEXCOORD3 = TEXCOORD2 << 1,

	BLEND_INDICES = TEXCOORD3 << 1,
	BLEND_WEIGHTS = BLEND_INDICES << 1,

	END = BLEND_WEIGHTS << 1,

	TERRAIN = 1,
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

enum class DESCRIPTOR_TYPE {
	CBV, 
	SRV, 
	UAV, 
	RTV, 
	DSV, 
	SAMPLER 
};

enum class WEAPON_TYPE : uint8 {
	M4 = 0,
	AK,
	RIFLE,
	PISTOL,
	MELEE,

	COUNT,

	UNDEFINED = 99
};

//////////////////////////////////////////////////////////////////////////////////
// Structs

struct EffectParameter {
	Vector3		xmf3Position;
	float		fElapsedTime = 0.f;
	Vector3		xmf3Force;
	float		fAdditionalData = 0.f;
};

struct Bone {
	std::string strBoneName;
	int nIndex;
	int nParentIndex;
	Matrix mtxTransform;
	Matrix mtxOffset;

	int nChildren = 0;
	int nDepth = 0;
	std::vector<int> nChilerenIndex;

	std::shared_ptr<IGameObject> pNode;
};

struct PendingUploadBuffer {
	ComPtr<ID3D12Resource> pd3dPendingUploadBuffer = nullptr;
	CommandListPair* cmdListPair;	// Only for ref
};

struct TerrainHit {
	float fHeight = 0.f;
	Vector3 v3Normal = Vector3{ 0,0,0 };
	float fPenetrationDepth = 0.f;
	bool bGrounded = false;
};

struct CollisionResult {
	std::shared_ptr<IGameObject> pSelf;
	std::shared_ptr<IGameObject> pOther;

	bool operator==(const CollisionResult& other) const noexcept {
		return (pSelf == other.pSelf) && (pOther == other.pOther);
	}

	// Helpers
	float GetDistanceBetweenCenter() const;
	Vector3 GetDirectionToOther() const;
	std::pair<const ICollider&, const ICollider&> DecomposeRef() const;

};

template<>
struct std::hash<CollisionResult> {
	size_t operator()(const CollisionResult& result) const {
		// hash combine
		size_t h1 = std::hash<std::shared_ptr<IGameObject>>{}(result.pSelf);
		size_t h2 = std::hash<std::shared_ptr<IGameObject>>{}(result.pOther);

		return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
	}
};

struct AnimationKey {
	Vector3 v3Translation{ 0.f, 0.f, 0.f };
	Quaternion v4RotationQuat{ 0.f, 0.f, 0.f, 1.f };
	Vector3 v3Scale{ 1.f, 1.f, 1.f };

	AnimationKey() = default;
	AnimationKey(const Vector3& t, const Quaternion& r, const Vector3& s) : v3Translation{ t }, v4RotationQuat{ r }, v3Scale{ s } {}
	//AnimationKey(const Vector3& t, const Vector3& r, const Vector3& s) : v3Translation{ t }, v4RotationQuat{ r }, v3Scale{ s } {}

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

template<typename T>
struct IteratorRange {
	T::const_iterator beginIt;
	T::const_iterator endIt;

	T::const_pointer begin() {
		return &(*beginIt);
	}

	T::const_pointer end() {
		return &(*endIt);
	}

	size_t distance() {
		return endIt - beginIt;
	}
};

#define ITERATOR_ARGS(itRange) itRange.beginIt, itRange.endIt

//////////////////////////////////////////////////////////////////////////////////
// Default Constants

constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAG_DEFAULT = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

inline const Matrix g_mtxToTexture = Matrix{
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, -0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.0f, 1.0f
};

//////////////////////////////////////////////////////////////////////////////////
// Type Aliasing
