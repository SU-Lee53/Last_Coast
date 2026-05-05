#include "pch.h"
#include "MuzzleFlashEffect.h"
#include "ParticleModules.h"

void MuzzleFlashEffect::Initialize()
{
	CreateCoreEmitter();
	CreateShapeEmitter();
}

void MuzzleFlashEffect::CreateCoreEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "MuzzleFlash_Core";
	desc.unMaxParticles = 8;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.08f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(3);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.035f, 0.07f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		10.0f, 18.0f,   // start size min/max
		2.0f, 5.0f    // end size min/max
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.85f, 0.35f, 1.0f),
		Vector4(1.0f, 0.25f, 0.04f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void MuzzleFlashEffect::CreateShapeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "MuzzleFlash_Shape";
	desc.unMaxParticles = 4;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_SHAPE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.09f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(2);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.04f, 0.085f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		22.0f, 40.0f,   // start size min/max
		4.0f, 10.0f    // end size min/max
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.70f, 0.25f, 0.85f),
		Vector4(1.0f, 0.18f, 0.02f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}
