#include "pch.h"
#include "ParticleEmitter.h"

ParticleEmitter::ParticleEmitter(const ParticleEmitterDesc& desc)
{
	Initialize(desc);
}

void ParticleEmitter::Initialize(const ParticleEmitterDesc& desc)
{
	m_EmitterDesc = desc;
	m_fAge = 0.f;
	m_bDead = false;

	m_Particles.clear();
	m_Particles.resize(m_EmitterDesc.unMaxParticles);

	for (auto& particle : m_Particles) {
		particle.bAlive = false;
	}
}

void ParticleEmitter::Update(const ParticleEffectSpawnDesc& spawnDesc)
{
	if (m_bDead) {
		return;
	}

	m_fAge += DT;

	ParticleModuleContext context{
		.fEmitterAge = m_fAge,
		.pSpawnDesc = &spawnDesc
	};

	// 1. Update alive particles
	bool bAnyAlive = false;
	for (auto& particle : m_Particles) {
		if (!particle.bAlive) {
			continue;
		}

		for (auto& pUpdateModule : m_pUpdateModules) {
			pUpdateModule->Update(particle, context);
		}

		if (particle.fAge >= particle.fLifetime) {
			particle.bAlive = false;
			continue;
		}

		bAnyAlive = true;
	}

	// 2. Spawn new
	for (auto& pSpawnModule : m_pSpawnModules) {
		pSpawnModule->UpdateSpawn(*this, context);
	}

	// 3, Check alive again after spawning new particles
	for (const auto& particle : m_Particles) {
		if (particle.bAlive) {
			bAnyAlive = true;
			break;
		}
	}

	// 4. Check emitter death
	if (!m_EmitterDesc.bLoop && m_fAge >= m_EmitterDesc.fEmitterLifetime && !bAnyAlive) {
		m_bDead = true;
	}
}

void ParticleEmitter::Emit(uint32 unCount, const ParticleModuleContext& context)
{
	for (uint32 i = 0; i < unCount; ++i) {
		Particle* pParticle = FindDeadParticle();
		if (!pParticle) {
			return;
		}

		*pParticle = Particle{};
		pParticle->bAlive = true;

		for (auto& pInitModule : m_pInitializeModules) {
			pInitModule->Initialize(*pParticle, context);
		}
	}
}

void ParticleEmitter::CollectRenderData(std::vector<ParticleRenderBatch>& outRenderBatches, const ParticleEffectSpawnDesc& spawnDesc) const
{
	ParticleRenderBatch batch{
		.textureRef = m_EmitterDesc.textureRef,
		.eBlendMode = m_EmitterDesc.eBlendMode,
		.eSortMode = m_EmitterDesc.eSortMode
	};

	for (const auto& particle : m_Particles) {
		if (!particle.bAlive) {
			continue;
		}

		ParticleDrawData drawData{
			.v3Position = particle.v3Position,
			.fSize = particle.fSize,
			.v4Color = particle.v4Color,
			.v4UVRect = particle.v4UVRect,
			.fRotation = particle.fRotation,
			.nTextureIndex = particle.nTextureIndex,
			.fSoftParticleFactor = 1.f
		};

		if (m_EmitterDesc.eSpace == PARTICLE_SPACE::LOCAL) {
			drawData.v3Position = Vector3::Transform(drawData.v3Position, spawnDesc.mtxWorld);
		}

		batch.drawDatas.push_back(drawData);
	}

	if (!batch.drawDatas.empty()) {
		outRenderBatches.emplace_back(std::move(batch));
	}
}

Particle* ParticleEmitter::FindDeadParticle()
{
	auto it = std::find_if(m_Particles.begin(), m_Particles.end(), [](const Particle& p) { return !p.bAlive; });
	return (it != m_Particles.end()) ? &(*it) : nullptr;
}
