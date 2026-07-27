#include "pch.h"
#include "DecoyExplosionEffect.h"
#include "ParticleModules.h"

void DecoyExplosionEffect::Initialize()
{
	CreateFlashEmitter();

	// 9색 폭죽 스파크 — 시작색은 additive 발광용으로 밝게(5배), 끝색까지 색을 유지해 또렷하게
	CreateSparkEmitter("DecoyExplosion_SparkRed",
		Vector4(1.00f, 0.10f, 0.10f, 1.0f) * 5.0f, Vector4(0.70f, 0.03f, 0.03f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkOrange",
		Vector4(1.00f, 0.55f, 0.05f, 1.0f) * 5.0f, Vector4(0.70f, 0.30f, 0.02f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkYellow",
		Vector4(1.00f, 0.95f, 0.15f, 1.0f) * 5.0f, Vector4(0.70f, 0.60f, 0.05f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkLime",
		Vector4(0.55f, 1.00f, 0.10f, 1.0f) * 5.0f, Vector4(0.30f, 0.70f, 0.03f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkGreen",
		Vector4(0.10f, 1.00f, 0.25f, 1.0f) * 5.0f, Vector4(0.03f, 0.70f, 0.10f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkCyan",
		Vector4(0.10f, 0.95f, 1.00f, 1.0f) * 5.0f, Vector4(0.03f, 0.55f, 0.70f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkBlue",
		Vector4(0.15f, 0.35f, 1.00f, 1.0f) * 5.0f, Vector4(0.05f, 0.15f, 0.70f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkMagenta",
		Vector4(1.00f, 0.15f, 0.95f, 1.0f) * 5.0f, Vector4(0.65f, 0.04f, 0.60f, 0.0f));
	CreateSparkEmitter("DecoyExplosion_SparkPink",
		Vector4(1.00f, 0.40f, 0.60f, 1.0f) * 5.0f, Vector4(0.70f, 0.20f, 0.35f, 0.0f));

	// 색연기 4색 — 채도 높인 컬러 연막이 사방으로 퍼짐
	CreateTintedSmokeEmitter("DecoyExplosion_SmokePink",   Vector4(1.00f, 0.35f, 0.65f, 0.55f));
	CreateTintedSmokeEmitter("DecoyExplosion_SmokeMint",   Vector4(0.30f, 1.00f, 0.60f, 0.55f));
	CreateTintedSmokeEmitter("DecoyExplosion_SmokeSky",    Vector4(0.30f, 0.65f, 1.00f, 0.55f));
	CreateTintedSmokeEmitter("DecoyExplosion_SmokeYellow", Vector4(1.00f, 0.85f, 0.25f, 0.55f));

	// 잔반짝임 3색 — 금색/시안/마젠타 트윙클
	CreateTwinkleEmitter("DecoyExplosion_TwinkleGold",
		Vector4(1.00f, 0.90f, 0.40f, 1.0f) * 6.0f, Vector4(0.60f, 0.45f, 0.15f, 0.0f));
	CreateTwinkleEmitter("DecoyExplosion_TwinkleCyan",
		Vector4(0.35f, 0.95f, 1.00f, 1.0f) * 6.0f, Vector4(0.10f, 0.50f, 0.60f, 0.0f));
	CreateTwinkleEmitter("DecoyExplosion_TwinkleMagenta",
		Vector4(1.00f, 0.30f, 0.90f, 1.0f) * 6.0f, Vector4(0.55f, 0.10f, 0.50f, 0.0f));

	if (!m_pSound) {
		m_pSound = SOUND->GetSound("explosion");
	}
}

void DecoyExplosionEffect::CreateFlashEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "DecoyExplosion_Flash";
	desc.unMaxParticles = 4;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.1f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(2);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 6.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.05f, 0.1f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		2.5f * 100, 4.0f * 100,
		1.0f * 100, 1.5f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 1.0f, 1.0f, 1.0f) * 6.0f,
		Vector4(0.8f, 0.8f, 1.0f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void DecoyExplosionEffect::CreateSparkEmitter(const char* name, const Vector4& colorStart, const Vector4& colorEnd)
{
	ParticleEmitterDesc desc{};
	desc.strName = name;
	desc.unMaxParticles = 14;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.1f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(12);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 10.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.5f, 1.0f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		0.3f * 100, 0.6f * 100,
		0.05f * 100, 0.15f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(colorStart, colorEnd);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitSphereVelocityModule>(150.0f, 450.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -250.0f, 0.0f));	// 폭죽처럼 포물선 낙하
	emitter.AddUpdateModule<DragModule>(1.5f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void DecoyExplosionEffect::CreateTintedSmokeEmitter(const char* name, const Vector4& color)
{
	ParticleEmitterDesc desc{};
	desc.strName = name;
	desc.unMaxParticles = 8;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.6f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(6);

	emitter.AddInitializeModule<InitPositionSphereModule>(5.f, 20.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.8f, 1.5f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		1.5f * 100, 2.5f * 100,
		3.5f * 100, 5.5f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		color,
		Vector4(color.x * 0.4f, color.y * 0.4f, color.z * 0.4f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-1.0f, 1.0f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(30.0f, 90.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 40.0f, 0.0f));	// 색연기는 살짝 떠오름
	emitter.AddUpdateModule<DragModule>(1.8f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void DecoyExplosionEffect::CreateTwinkleEmitter(const char* name, const Vector4& colorStart, const Vector4& colorEnd)
{
	ParticleEmitterDesc desc{};
	desc.strName = name;
	desc.unMaxParticles = 16;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.4f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	// 폭발 직후 0.7초간 흩뿌려지는 잔반짝임 — 폭죽 여운
	emitter.AddSpawnModule<RateSpawnModule>(20.0f, 0.7f);

	emitter.AddInitializeModule<InitPositionSphereModule>(20.f, 130.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.15f, 0.4f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		0.15f * 100, 0.35f * 100,
		0.02f * 100, 0.08f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(colorStart, colorEnd);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void DecoyExplosionEffect::Play(const ParticleEffectSpawnDesc& desc)
{
	IParticleEffect::Play(desc);
	SOUND->PlayAt(m_pSound, desc.v3Position);	// "explosion"은 3D 등록 — 폭발 지점에서 재생
}
