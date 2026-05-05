#pragma once
#include "ParticleTypes.h"

class ParticleEmitter;
struct ParticleModuleContext;

interface IParticleSpawnModule abstract{
public:
	virtual ~IParticleSpawnModule() = default;

	virtual void UpdateSpawn(
		ParticleEmitter& emitter, 
		const ParticleModuleContext& context) = 0;
};

interface IParticleInitializeModule abstract{
public:
	virtual ~IParticleInitializeModule() = default;

	virtual void Initialize(
		Particle& particle,
		const ParticleModuleContext& context) = 0;
};

interface IParticleUpdateModule abstract{
public:
	virtual ~IParticleUpdateModule() = default;

	virtual void Update(
		Particle& particle,
		const ParticleModuleContext& context) = 0;
};
