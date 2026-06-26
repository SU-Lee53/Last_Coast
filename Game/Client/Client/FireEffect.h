#pragma once
#include "ParticleEffect.h"

class FireEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateFlameEmitter();
	void CreateHeatCoreEmitter();
	void CreateSmokeEmitter();
};

