#pragma once
#include "ParticleModule.h"
#include "ParticleEmitter.h"

///////////////////////////////////////////////////////////////////////////
// Spawn Modules

class BurstSpawnModule : public IParticleSpawnModule {
public:
	explicit BurstSpawnModule(uint32 unCount)
		: m_unCount(unCount) {}

	virtual void UpdateSpawn(
		ParticleEmitter& emitter,
		const ParticleModuleContext& context) override;

private:
	uint32 m_unCount = 0;
	bool m_bSpawned = false;
};

///////////////////////////////////////////////////////////////////////////
// Initialize Modules

class InitPositionFromEffectModule : public IParticleInitializeModule {
public:
	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;
};

class InitLifetimeRandomModule : public IParticleInitializeModule {
public:
	InitLifetimeRandomModule(float fMin, float fMax)
		: m_fMin(fMin), m_fMax(fMax) {}

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	float m_fMin = 1.f;
	float m_fMax = 1.f;
};

class InitSizeRandomModule : public IParticleInitializeModule {
public:
	InitSizeRandomModule(float fStartMin, float fStartMax, float fEndMin, float fEndMax)
		: m_fStartMin(fStartMin), m_fStartMax(fStartMax), m_fEndMin(fEndMin), m_fEndMax(fEndMax) {}

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	float m_fStartMin = 1.f;
	float m_fStartMax = 1.f;
	float m_fEndMin = 1.f;
	float m_fEndMax = 1.f;
};

class InitColorModule : public IParticleInitializeModule {
public:
	InitColorModule(const Vector4& v4StartColor, const Vector4& v4EndColor)
		: m_v4StartColor(v4StartColor), m_v4EndColor(v4EndColor) {}

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	Vector4 m_v4StartColor = Vector4::One;
	Vector4 m_v4EndColor = Vector4::One;
};

class InitRandomRotationModule : public IParticleInitializeModule {
public:
	InitRandomRotationModule(float fMin, float fMax)
		: m_fMin(fMin), m_fMax(fMax) {}

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	float m_fMin = 0.f;
	float m_fMax = XM_2PI;
};

class InitConeVelocityModule : public IParticleInitializeModule {
public:
	InitConeVelocityModule(float fSpeedMin, float fSpeedMax, float fConeAngleRadians)
		: m_fSpeedMin(fSpeedMin), m_fSpeedMax(fSpeedMax), m_fConeAngleRadians(fConeAngleRadians) {}

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	float m_fSpeedMin = 0.f;
	float m_fSpeedMax = 1.f;
	float m_fConeAngleRadians = XM_PIDIV4;
};

///////////////////////////////////////////////////////////////////////////
// Update Modules

class UpdateAgeModule : public IParticleUpdateModule {
public:
	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;
};

class UpdateVelocityModule : public IParticleUpdateModule {
public:
	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;
};

class GravityModule : public IParticleUpdateModule {
public:
	explicit GravityModule(const Vector3& v3Gravity)
		: m_v3Gravity(v3Gravity) {}

	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	Vector3 m_v3Gravity = Vector3(0.f, -9.8f, 0.f);
};

class DragModule : public IParticleUpdateModule {
public:
	explicit DragModule(float fDrag)
		: m_fDrag(fDrag) {
	}

	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;

private:
	float m_fDrag = 0.f;
};

class UpdateSizeOverLifeModule : public IParticleUpdateModule {
public:
	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;
};

class UpdateColorOverLifeModule : public IParticleUpdateModule {
public:
	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;
};

class UpdateRotationModule : public IParticleUpdateModule {
public:
	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) override;
};
