#include "pch.h"
#include "ExplosionEffect.h"
#include "ParticleModules.h"

void ExplosionEffect::Initialize()
{
	CreateFlashEmitter();
	CreateFireballEmitter();
	CreateSmokeEmitter();
	CreateDustEmitter();

	if (!m_pSound) {
		SOUND->GetSound("explosion");
	}
}

void ExplosionEffect::CreateFlashEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Explosion_Flash";
	desc.unMaxParticles = 8;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.12f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(4);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 18.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.06f, 0.12f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		60.0f * 100, 110.0f * 100,
		20.0f * 100, 40.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.80f, 0.35f, 1.0f) * 7.0f,
		Vector4(1.0f, 0.20f, 0.03f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void ExplosionEffect::CreateFireballEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Explosion_Fireball";
	desc.unMaxParticles = 48;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::EXPLOSION_FIREBALL);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.55f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(30);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 35.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.25f, 0.55f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		28.0f * 100, 55.0f * 100,
		75.0f * 100, 135.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.55f, 0.12f, 0.95f) * 3.5f,
		Vector4(0.75f, 0.08f, 0.01f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-5.0f, 5.0f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(90.0f, 260.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 45.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.4f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void ExplosionEffect::CreateSmokeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Explosion_Smoke";
	desc.unMaxParticles = 80;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 2.8f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(34);
	emitter.AddSpawnModule<RateSpawnModule>(16.0f, 0.8f);

	emitter.AddInitializeModule<InitPositionSphereModule>(12.f, 65.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(1.2f, 2.6f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		35.0f * 100, 80.0f * 100,
		130.0f * 100, 260.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.30f, 0.28f, 0.24f, 0.45f),
		Vector4(0.08f, 0.08f, 0.08f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-1.2f, 1.2f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(40.0f, 130.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 65.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.6f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void ExplosionEffect::CreateDustEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Explosion_Dust";
	desc.unMaxParticles = 56;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::IMPACT_DUST);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.5f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(40);

	emitter.AddInitializeModule<InitPositionSphereModule>(10.f, 45.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.65f, 1.35f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		18.0f * 100, 42.0f * 100,
		70.0f * 100, 150.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.48f, 0.42f, 0.34f, 0.55f),
		Vector4(0.20f, 0.18f, 0.16f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-2.0f, 2.0f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(120.0f, 340.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -180.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.8f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void ExplosionEffect::Play(const ParticleEffectSpawnDesc& desc)
{
	IParticleEffect::Play(desc);
	SOUND->Play(m_pSound);
}
