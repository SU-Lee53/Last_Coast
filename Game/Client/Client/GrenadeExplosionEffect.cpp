#include "pch.h"
#include "GrenadeExplosionEffect.h"
#include "ParticleModules.h"

void GrenadeExplosionEffect::Initialize()
{
	CreateFlashEmitter();
	CreateFireballEmitter();
	CreateSmokeEmitter();
	CreateDustEmitter();

	if (!m_pSound) {
		m_pSound = SOUND->GetSound("explosion");
	}
}

void GrenadeExplosionEffect::CreateFlashEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "GrenadeExplosion_Flash";
	desc.unMaxParticles = 6;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.1f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(3);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 8.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.05f, 0.1f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		4.0f * 100, 7.0f * 100,
		1.5f * 100, 2.5f * 100
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

void GrenadeExplosionEffect::CreateFireballEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "GrenadeExplosion_Fireball";
	desc.unMaxParticles = 24;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::EXPLOSION_FIREBALL);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.45f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(16);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 12.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.2f, 0.45f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		2.0f * 100, 3.5f * 100,
		4.5f * 100, 7.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.55f, 0.12f, 0.95f) * 3.5f,
		Vector4(0.75f, 0.08f, 0.01f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-5.0f, 5.0f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(60.0f, 170.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 35.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.8f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void GrenadeExplosionEffect::CreateSmokeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "GrenadeExplosion_Smoke";
	desc.unMaxParticles = 40;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.8f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(18);
	emitter.AddSpawnModule<RateSpawnModule>(10.0f, 0.5f);

	emitter.AddInitializeModule<InitPositionSphereModule>(5.f, 22.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.8f, 1.7f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		2.5f * 100, 4.5f * 100,
		6.5f * 100, 11.0f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.30f, 0.28f, 0.24f, 0.45f),
		Vector4(0.08f, 0.08f, 0.08f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-1.2f, 1.2f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(25.0f, 85.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 45.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.6f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void GrenadeExplosionEffect::CreateDustEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "GrenadeExplosion_Dust";
	desc.unMaxParticles = 32;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::IMPACT_DUST);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.0f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(22);

	emitter.AddInitializeModule<InitPositionSphereModule>(4.f, 16.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.5f, 1.0f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		1.2f * 100, 2.5f * 100,
		4.0f * 100, 7.5f * 100
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.48f, 0.42f, 0.34f, 0.55f),
		Vector4(0.20f, 0.18f, 0.16f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-2.0f, 2.0f);
	emitter.AddInitializeModule<InitSphereVelocityModule>(90.0f, 240.0f);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -180.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.8f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void GrenadeExplosionEffect::Play(const ParticleEffectSpawnDesc& desc)
{
	IParticleEffect::Play(desc);
	SOUND->PlayAt(m_pSound, desc.v3Position);	// "explosion"은 3D 등록 — 폭발 지점에서 재생
}
