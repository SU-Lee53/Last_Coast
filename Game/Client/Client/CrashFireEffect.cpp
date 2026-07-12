#include "pch.h"
#include "CrashFireEffect.h"
#include "ParticleModules.h"

// FireEffect 대비 배율 — 크기/반경/속도에 적용. 수명/리스폰 주기는 동일 유지
// (CrashSiteFireEvent 의 FIRE_EFFECT_ESTIMATED_DURATION 가정이 그대로 성립).
namespace
{
	constexpr float FIRE_SCALE = 30.f;
}

void CrashFireEffect::Initialize()
{
	CreateFlameEmitter();
	CreateHeatCoreEmitter();
	CreateSmokeEmitter();
}

void CrashFireEffect::CreateFlameEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "CrashFire_Flame";
	desc.unMaxParticles = 160;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::FIRE_FLAME);
	desc.bLoop = false;
	desc.fEmitterLifetime = 4.2f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(48.0f, 3.2f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 22.f * FIRE_SCALE);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.45f, 1.0f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		18.0f * FIRE_SCALE, 36.0f * FIRE_SCALE,
		38.0f * FIRE_SCALE, 76.0f * FIRE_SCALE
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.58f, 0.12f, 0.85f) * 2.8f,
		Vector4(0.65f, 0.04f, 0.01f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-4.0f, 4.0f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		80.0f * FIRE_SCALE * 0.6f,
		185.0f * FIRE_SCALE * 0.6f,
		XMConvertToRadians(28.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 120.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(2.2f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}

void CrashFireEffect::CreateHeatCoreEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "CrashFire_HeatCore";
	desc.unMaxParticles = 64;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	desc.eSortMode = PARTICLE_SORT_MODE::NONE;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::MUZZLE_FLASH_CORE);
	desc.bLoop = false;
	desc.fEmitterLifetime = 3.6f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(24.0f, 3.0f);

	emitter.AddInitializeModule<InitPositionSphereModule>(0.f, 12.f * FIRE_SCALE);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(0.22f, 0.5f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		16.0f * FIRE_SCALE, 28.0f * FIRE_SCALE,
		6.0f * FIRE_SCALE, 14.0f * FIRE_SCALE
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(1.0f, 0.82f, 0.35f, 1.0f) * 4.0f,
		Vector4(1.0f, 0.25f, 0.02f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		40.0f * FIRE_SCALE * 0.6f,
		95.0f * FIRE_SCALE * 0.6f,
		XMConvertToRadians(35.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<DragModule>(2.5f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
}

void CrashFireEffect::CreateSmokeEmitter()
{
	ParticleEmitterDesc desc{};
	desc.strName = "CrashFire_Smoke";
	desc.unMaxParticles = 110;
	desc.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	desc.eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;
	desc.eSpace = PARTICLE_SPACE::WORLD;
	desc.textureRef = PARTICLE->GetTextureCached(PARTICLE_TEXTURE_ID::SMOKE_PUFF);
	desc.bLoop = false;
	desc.fEmitterLifetime = 5.0f;

	auto& emitter = AddEmitter<ParticleEmitter>(desc);

	emitter.AddSpawnModule<RateSpawnModule>(20.0f, 3.2f);

	emitter.AddInitializeModule<InitPositionSphereModule>(4.f * FIRE_SCALE, 24.f * FIRE_SCALE);
	emitter.AddInitializeModule<InitLifetimeRandomModule>(1.6f, 2.8f);
	emitter.AddInitializeModule<InitSizeRandomModule>(
		24.0f * FIRE_SCALE, 48.0f * FIRE_SCALE,
		95.0f * FIRE_SCALE, 180.0f * FIRE_SCALE
	);
	emitter.AddInitializeModule<InitColorModule>(
		Vector4(0.18f, 0.16f, 0.14f, 0.32f),
		Vector4(0.06f, 0.06f, 0.06f, 0.0f)
	);
	emitter.AddInitializeModule<InitRandomRotationModule>(0.f, XM_2PI);
	emitter.AddInitializeModule<InitAngularVelocityRandomModule>(-0.9f, 0.9f);
	emitter.AddInitializeModule<InitConeVelocityModule>(
		35.0f * FIRE_SCALE * 0.7f,
		95.0f * FIRE_SCALE * 0.7f,
		XMConvertToRadians(34.0f)
	);

	emitter.AddUpdateModule<UpdateAgeModule>();
	emitter.AddUpdateModule<UpdateVelocityModule>();
	emitter.AddUpdateModule<GravityModule>(Vector3(0.0f, 100.0f, 0.0f));
	emitter.AddUpdateModule<DragModule>(1.2f);
	emitter.AddUpdateModule<UpdateSizeOverLifeModule>();
	emitter.AddUpdateModule<UpdateColorOverLifeModule>();
	emitter.AddUpdateModule<UpdateRotationModule>();
}
