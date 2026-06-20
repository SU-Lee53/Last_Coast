#include "pch.h"
#include "FireEffect.h"
#include "ParticleModules.h"

void FireEffect::Initialize()
{
	CreateFlameEmitter();
	CreateHeatCoreEmitter();
	CreateSmokeEmitter();
}

void FireEffect::CreateFlameEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Fire_Flame";
	desc.unMaxParticles = 96;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::FIRE_FLAME);
	desc.bLoop = false;
	desc.fEmitterLifetime = 4.2f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(34.0f, 3.2f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 22.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.38f, 0.85f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		18.0f, 36.0f,
		38.0f, 76.0f
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.58f, 0.12f, 0.85f) * 2.8f,
		Vector4(0.65f, 0.04f, 0.01f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-4.0f, 4.0f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		80.0f,
		185.0f,
		XMConvertToRadians(28.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 80.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.2f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void FireEffect::CreateHeatCoreEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Fire_HeatCore";
	desc.unMaxParticles = 48;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 3.6f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(18.0f, 3.0f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 12.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.18f, 0.42f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		16.0f, 28.0f,
		6.0f, 14.0f
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.82f, 0.35f, 1.0f) * 4.0f,
		Vector4(1.0f, 0.25f, 0.02f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		40.0f,
		95.0f,
		XMConvertToRadians(35.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<DragModule>(2.5f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void FireEffect::CreateSmokeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Fire_Smoke";
	desc.unMaxParticles = 72;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 5.0f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(15.0f, 3.2f);

	emitter.AddInitializeModule<InitPositionSphereModule>(4.f, 24.f);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(1.2f, 2.2f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		24.0f, 48.0f,
		95.0f, 180.0f
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.18f, 0.16f, 0.14f, 0.32f),
		Vector4(0.06f, 0.06f, 0.06f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-0.9f, 0.9f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		35.0f,
		95.0f,
		XMConvertToRadians(34.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 70.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.2f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}
