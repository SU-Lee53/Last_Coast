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
};

