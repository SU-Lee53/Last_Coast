#pragma once
#include "ParticleRenderBatch.h"
#include "ParticleEmitter.h"

class ParticleEmitter;
struct ParticleRenderBatch;

interface IParticleEffect abstract {
public:
	virtual ~IParticleEffect() = default;

	virtual void Initialize() = 0;
	virtual void Update();

	virtual void Play(const ParticleEffectSpawnDesc& desc);

	virtual void CollectRenderData(std::vector<ParticleRenderBatch>& outRenderBatches) const;

	bool IsDead() const { return m_bDead; }
	bool IsPlaying() const { return m_bPlaying; }

protected:
	template <typename T, typename... Args>
	T& AddEmitter(Args&&... args) {
		auto pEmitter = std::make_unique<T>(std::forward<Args>(args)...);
		T& ref = *pEmitter;
		m_pEmitters.emplace_back(std::move(pEmitter));
		return ref;
	}

protected:
	ParticleEffectSpawnDesc m_SpawnDesc{};
	std::vector<std::unique_ptr<class ParticleEmitter>> m_pEmitters;

	bool m_bPlaying = false;
	bool m_bDead = false;

};

