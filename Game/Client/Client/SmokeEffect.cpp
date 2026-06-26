#include "pch.h"
#include "SmokeEffect.h"
#include "ParticleModules.h"

void SmokeEffect::Initialize()
{
	CreatePuffEmitter();
	CreateTrailEmitter();
}

void SmokeEffect::CreatePuffEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Smoke_Puff";
	desc.unMaxParticles = 72;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 4.2f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(18);
	emitter.AddSpawnModule<RateSpawnModule>(12.0f, 2.3f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 38.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(1.6f, 3.4f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		38.0f, 85.0f,
		140.0f, 300.0f
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.33f, 0.33f, 0.32f, 0.42f),
		Vector4(0.12f, 0.12f, 0.12f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-0.7f, 0.7f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		28.0f,
		90.0f,
		XMConvertToRadians(42.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 55.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.0f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void SmokeEffect::CreateTrailEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Smoke_Trail";
	desc.unMaxParticles = 40;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::IMPACT_DUST);
	desc.bLoop = false;
	desc.fEmitterLifetime = 2.8f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(10.0f, 1.6f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 22.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(1.0f, 2.1f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		20.0f, 46.0f,
		80.0f, 165.0f
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.24f, 0.23f, 0.21f, 0.30f),
		Vector4(0.10f, 0.10f, 0.10f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-0.5f, 0.5f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		20.0f,
		70.0f,
		XMConvertToRadians(50.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 38.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.4f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

