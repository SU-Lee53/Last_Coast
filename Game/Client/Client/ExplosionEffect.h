#pragma once
#include "ParticleEffect.h"

class ExplosionEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateFlashEmitter();
	void CreateFireballEmitter();
	void CreateSmokeEmitter();
	void CreateDustEmitter();

	virtual void Play(const ParticleEffectSpawnDesc& desc) override;

private:
	std::shared_ptr<Sound> m_pSound = nullptr;

};

