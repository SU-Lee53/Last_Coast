#include "pch.h"
#include "BloodEffect.h"
#include "ParticleModules.h"

void BloodEffect::Initialize()
{
	CreateMistEmitter();
	CreateDropEmitter();
	CreateSplatterEmitter();
}

void BloodEffect::CreateMistEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Blood_Mist";
	desc.unMaxParticles = 48;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::BLOOD_MIST);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.7f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(24);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.25f, 0.55f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		8.0f, 18.0f,    // start half-size: 지름 16~36cm
		16.0f, 32.0f    // end half-size: 지름 32~64cm
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.55f, 0.02f, 0.02f, 0.65f),
		Vector4(0.18f, 0.0f, 0.0f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	// 피격 방향으로 퍼짐
	emitter.AddInitializeModule<InitConeVelocityModule>(
		40.0f,              // 40cm/s
		180.0f,             // 180cm/s
		XMConvertToRadians(55.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -120.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.0f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void BloodEffect::CreateDropEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Blood_Drop";
	desc.unMaxParticles = 32;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::BLOOD_DROP);
	desc.bLoop = false;
	desc.fEmitterLifetime = 1.2f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(12);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.45f, 1.0f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		1.5f, 4.0f,     // start half-size: 지름 3~8cm
		1.0f, 2.5f      // end half-size
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.45f, 0.0f, 0.0f, 0.95f),
		Vector4(0.12f, 0.0f, 0.0f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddInitializeModule<InitConeVelocityModule>(
		100.0f,
		360.0f,
		XMConvertToRadians(40.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -980.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(0.25f);
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void BloodEffect::CreateSplatterEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "Blood_Splatter_Small";
	desc.unMaxParticles = 20;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::BLOOD_SPLATTER_SMALL);
	desc.bLoop = false;
	desc.fEmitterLifetime = 0.8f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<BurstSpawnModule>(8);

	emitter.AddInitializeModule<InitPositionFromEffectModule>();
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.25f, 0.7f);

	emitter.AddInitializeModule<InitSizeRandomModule>(
		3.0f, 8.0f,
		2.0f, 5.0f
	);

	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.50f, 0.0f, 0.0f, 0.85f),
		Vector4(0.15f, 0.0f, 0.0f, 0.0f)
	);

	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);

	emitter.AddInitializeModule<InitConeVelocityModule>(
		80.0f,
		280.0f,
		XMConvertToRadians(70.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, -500.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(0.8f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}
