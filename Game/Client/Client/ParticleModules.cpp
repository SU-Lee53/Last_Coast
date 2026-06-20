#include "pch.h"
#include "ParticleModules.h"

///////////////////////////////////////////////////////////////////////////
// Spawn Modules

void BurstSpawnModule::UpdateSpawn(ParticleEmitter& emitter, const ParticleModuleContext& context)
{
	if (m_bSpawned) {
		return;
	}

	emitter.Emit(m_unCount, context);
	m_bSpawned = true;
}

void RateSpawnModule::UpdateSpawn(ParticleEmitter& emitter, const ParticleModuleContext& context)
{
	if (context.fEmitterAge > m_fDuration) {
		return;
	}

	m_fSpawnAccumulator += m_fSpawnRate * DT;

	const uint32 unSpawnCount = static_cast<uint32>(m_fSpawnAccumulator);
	if (unSpawnCount == 0) {
		return;
	}

	m_fSpawnAccumulator -= static_cast<float>(unSpawnCount);
	emitter.Emit(unSpawnCount, context);
}

///////////////////////////////////////////////////////////////////////////
// Initialize Modules


///////////////////////////////////////////////////////////////////////////
// InitPositionFromEffectModule Modules

void InitPositionFromEffectModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	if (!context.pSpawnDesc) {
		return;
	}

	particle.v3Position = context.pSpawnDesc->v3Position;
}

void InitPositionSphereModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	if (!context.pSpawnDesc) {
		return;
	}

	const float fRadius = RandomGenerator::GenerateRandomFloatInRange(m_fRadiusMin, m_fRadiusMax);
	Vector3 v3Offset = Vector3::Zero;
	XMStoreFloat3(&v3Offset, RandomGenerator::GenerateRandomUnitVectorOnSphere() * fRadius);

	particle.v3Position = context.pSpawnDesc->v3Position + v3Offset;
}

///////////////////////////////////////////////////////////////////////////
// InitLifetimeRandomModule Modules

void InitLifetimeRandomModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	particle.fAge = 0.f;
	particle.fLifetime = RandomGenerator::GenerateRandomFloatInRange(m_fMin, m_fMax);
}

///////////////////////////////////////////////////////////////////////////
// InitSizeRandomModule Modules

void InitSizeRandomModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	particle.fStartSize = RandomGenerator::GenerateRandomFloatInRange(m_fStartMin, m_fStartMax);
	particle.fEndSize = RandomGenerator::GenerateRandomFloatInRange(m_fEndMin, m_fEndMax);
	particle.fSize = particle.fStartSize;
}

///////////////////////////////////////////////////////////////////////////
// InitColorModule Modules

void InitColorModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	particle.v4StartColor = m_v4StartColor;
	particle.v4EndColor = m_v4EndColor;
	particle.v4Color = m_v4StartColor;
}

///////////////////////////////////////////////////////////////////////////
// InitRandomRotationModule Modules

void InitRandomRotationModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	particle.fRotation = RandomGenerator::GenerateRandomFloatInRange(m_fMin, m_fMax);
}

void InitAngularVelocityRandomModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	particle.fAngularVelocity = RandomGenerator::GenerateRandomFloatInRange(m_fMin, m_fMax);
}

///////////////////////////////////////////////////////////////////////////
// InitConeVelocityModule Modules

void InitConeVelocityModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	if (!context.pSpawnDesc) {
		return;
	}

	Vector3 dir = context.pSpawnDesc->v3Direction;
	dir.Normalize();

	Vector3 up = Vector3::Up;

	if (std::abs(dir.Dot(up)) > 0.95f) {
		up = Vector3::Right;
	}

	Vector3 right = up.Cross(dir);
	right.Normalize();

	Vector3 realUp = dir.Cross(right);
	realUp.Normalize();

	const float theta = RandomGenerator::GenerateRandomFloatInRange(0.f, XM_2PI);
	const float cone = RandomGenerator::GenerateRandomFloatInRange(0.f, m_fConeAngleRadians);

	const float sinCone = sinf(cone);
	const float cosCone = cosf(cone);

	Vector3 randomDir =
		dir * cosCone +
		right * (cosf(theta) * sinCone) +
		realUp * (sinf(theta) * sinCone);

	randomDir.Normalize();

	const float speed = RandomGenerator::GenerateRandomFloatInRange(m_fSpeedMin, m_fSpeedMax);
	particle.v3Velocity = randomDir * speed;
}

void InitSphereVelocityModule::Initialize(Particle& particle, const ParticleModuleContext& context)
{
	Vector3 v3Velocity = Vector3::Zero;
	const float fSpeed = RandomGenerator::GenerateRandomFloatInRange(m_fSpeedMin, m_fSpeedMax);
	XMStoreFloat3(&v3Velocity, RandomGenerator::GenerateRandomUnitVectorOnSphere() * fSpeed);
	particle.v3Velocity = v3Velocity;
}

///////////////////////////////////////////////////////////////////////////
// Update Modules


///////////////////////////////////////////////////////////////////////////
// UpdateAgeModule Modules

void UpdateAgeModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	particle.fAge += DT;
}

///////////////////////////////////////////////////////////////////////////
// UpdateVelocityModule Modules

void UpdateVelocityModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	particle.v3Velocity += particle.v3Acceleration * DT;
	particle.v3Position += particle.v3Velocity * DT;
}

///////////////////////////////////////////////////////////////////////////
// GravityModule Modules

void GravityModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	particle.v3Velocity += m_v3Gravity * DT;
}

///////////////////////////////////////////////////////////////////////////
// DragModule Modules

void DragModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	const float fScale = std::max(0.f, 1.f - m_fDrag * DT);
	particle.v3Velocity *= fScale;
}

///////////////////////////////////////////////////////////////////////////
// UpdateSizeOverLifeModule Modules

void UpdateSizeOverLifeModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	const float t = std::clamp(
		particle.fAge / std::max(particle.fLifetime, 0.0001f),
		0.f,
		1.f
	);

	particle.fSize = std::lerp(
		particle.fStartSize,
		particle.fEndSize,
		t
	);
}

///////////////////////////////////////////////////////////////////////////
// UpdateColorOverLifeModule Modules

void UpdateColorOverLifeModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	const float t = std::clamp(
		particle.fAge / std::max(particle.fLifetime, 0.0001f),
		0.f,
		1.f
	);

	particle.v4Color = Vector4::Lerp(
		particle.v4StartColor,
		particle.v4EndColor,
		t
	);
}

///////////////////////////////////////////////////////////////////////////
// UpdateGlowOverLifeModule Modules

void UpdateGlowOverLifeModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	const float t = std::clamp(
		particle.fAge / std::max(particle.fLifetime, 0.0001f),
		0.f,
		1.f
	);

	particle.v4Color = Vector4::Lerp(
		particle.v4StartColor,
		particle.v4EndColor,
		t
	);
}

///////////////////////////////////////////////////////////////////////////
// UpdateRotationModule Modules

void UpdateRotationModule::Update(Particle& particle, const ParticleModuleContext& context)
{
	particle.fRotation += particle.fAngularVelocity * DT;
}
