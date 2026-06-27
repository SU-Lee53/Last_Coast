#pragma once
#include "ParticleTypes.h"
#include "ParticleRenderBatch.h"
#include "ParticleModule.h"

struct ParticleModuleContext {
	float fEmitterAge = 0.f;
	const ParticleEffectSpawnDesc* pSpawnDesc = nullptr;
};

class ParticleEmitter {
public:
	ParticleEmitter() = default;
	ParticleEmitter(const ParticleEmitterDesc& desc);

	void Initialize(const ParticleEmitterDesc& desc);
	void Reset();
	void Update(const ParticleEffectSpawnDesc& spawnDesc);
	void Emit(uint32 unCount, const ParticleModuleContext& context);

	void CollectRenderData(
		std::vector<ParticleRenderBatch>& outRenderBatches,
		const ParticleEffectSpawnDesc& spawnDesc) const;

	bool IsDead() const { return m_bDead; }

	template<typename T, typename... Args> requires std::derived_from<T, IParticleSpawnModule>
	T& AddSpawnModule(Args&&... args);

	template<typename T, typename... Args> requires std::derived_from<T, IParticleInitializeModule>
	T& AddInitializeModule(Args&&... args);

	template<typename T, typename... Args> requires std::derived_from<T, IParticleUpdateModule>
	T& AddUpdateModule(Args&&... args);


private:
	Particle* FindDeadParticle();


private:
	ParticleEmitterDesc m_EmitterDesc{};

	std::vector<Particle> m_Particles;
	
	std::vector<std::unique_ptr<IParticleSpawnModule>> m_pSpawnModules;
	std::vector<std::unique_ptr<IParticleInitializeModule>> m_pInitializeModules;
	std::vector<std::unique_ptr<IParticleUpdateModule>> m_pUpdateModules;

	float m_fAge = 0.f;
	bool m_bDead = false;
};

template<typename T, typename... Args> requires std::derived_from<T, IParticleSpawnModule>
T& ParticleEmitter::AddSpawnModule(Args&&... args)
{
	auto pModule = std::make_unique<T>(std::forward<Args>(args)...);
	T& ref = *pModule;
	m_pSpawnModules.emplace_back(std::move(pModule));
	return ref;
}

template<typename T, typename... Args> requires std::derived_from<T, IParticleInitializeModule>
T& ParticleEmitter::AddInitializeModule(Args&&... args)
{
	auto pModule = std::make_unique<T>(std::forward<Args>(args)...);
	T& ref = *pModule;
	m_pInitializeModules.emplace_back(std::move(pModule));
	return ref;
}

template<typename T, typename... Args> requires std::derived_from<T, IParticleUpdateModule>
T& ParticleEmitter::AddUpdateModule(Args&&... args)
{
	auto pModule = std::make_unique<T>(std::forward<Args>(args)...);
	T& ref = *pModule;
	m_pUpdateModules.emplace_back(std::move(pModule));
	return ref;
}

