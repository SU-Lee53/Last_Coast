#include "pch.h"
#include "BulletImpactEffect.h"
#include "ParticleModules.h"

void BulletImpactEffect::Initialize()
{
	CreateDustEmitter();
	CreateSmokeEmitter();
}

void BulletImpactEffect::CreateDustEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Impact_Dust";
	desc.unMaxParticles = 32;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::IMPACT_DUST);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.6f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(18);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.25f, 0.55f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		4.0f, 10.0f,
		12.0f, 24.0f
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.55f, 0.50f, 0.42f, 0.65f),
		Vector4(0.30f, 0.28f, 0.25f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	// hit normal 방향으로 퍼지게
	emitter.AddInitializeModule<InitConeVelocityModule>(
		40.0f,
		180.0f,
		XMConvertToRadians(65.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -120.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.5f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void BulletImpactEffect::CreateSmokeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Impact_Smoke";
	desc.unMaxParticles = 12;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.8f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(6);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.45f, 0.8f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		8.0f, 16.0f,
		20.0f, 36.0f
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.45f, 0.43f, 0.40f, 0.35f),
		Vector4(0.25f, 0.25f, 0.25f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddInitializeModule<InitConeVelocityModule>(
		20.0f,
		80.0f,
		XMConvertToRadians(45.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 20.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.5f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}
