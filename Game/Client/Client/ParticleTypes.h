#pragma once

enum class PARTICLE_TEXTURE_ID : uint8 {
	MUZZLE_FLASH_CORE = 0,
	MUZZLE_FLASH_SHAPE,
	FIRE_FLAME,
	EXPLOSION_FIREBALL,

	BLOOD_MIST,
	BLOOD_DROP,
	BLOOD_SPLATTER_SMALL,

	SMOKE_PUFF,
	IMPACT_DUST,

	COUNT
};
enum class PARTICLE_BLEND_MODE : uint8 {
	ALPHA_BLEND,
	ADDITIVE
};

enum class PARTICLE_SORT_MODE : uint8 {
	NONE,
	BACK_TO_FRONT
};

enum class PARTICLE_SPACE : uint8 {
	LOCAL,
	WORLD
};

struct Particle {
	Vector3 v3Position = Vector3::Zero;
	Vector3 v3Velocity = Vector3::Zero;
	Vector3 v3Acceleration = Vector3::Zero;

	float fAge = 0.f;
	float fLifetime = 1.f;

	float fSize = 1.f;
	float fStartSize = 1.f;
	float fEndSize = 1.f;

	float fRotation = 0.f;
	float fAngularVelocity = 0.f;

	Vector4 v4Color = Vector4::One;
	Vector4 v4StartColor = Vector4::One;
	Vector4 v4EndColor = Vector4::One;

	Vector4 v4UVRect = Vector4(0.f, 0.f, 1.f, 1.f);

	int32 nTextureIndex = 0;
	bool bAlive = true;
};

struct ParticleDrawData {
	Vector3 v3Position = Vector3::Zero;
	float fSize = 1.f;

	Vector4 v4Color = Vector4::One;
	Vector4 v4UVRect = Vector4(0.f, 0.f, 1.f, 1.f);

	float fRotation = 0.f;
	int32 nTextureIndex = 0;
	float fSoftParticleFactor = 1.f;
	float pad0 = 0.f;
};

struct ParticleEmitterDesc {
	std::string strName;

	uint32 unMaxParticles = 128;

	PARTICLE_BLEND_MODE eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	PARTICLE_SORT_MODE eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	PARTICLE_SPACE eSpace = PARTICLE_SPACE::WORLD;

	TextureRef<Texture> textureRef;

	bool bLoop = false;
	float fEmitterLifetime = 1.f;
};

struct ParticleEffectSpawnDesc {
	Matrix mtxWorld = Matrix::Identity;

	Vector3 v3Position = Vector3::Zero;
	Vector3 v3Direction = Vector3::Backward;	// SimpleMath Vector3::Backward is forward in LHS
	Vector3 v3Normal = Vector3::Zero;
};
