#pragma once
#include "ParticleEffect.h"

class BulletImpactEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateDustEmitter();
	void CreateSmokeEmitter();
};

