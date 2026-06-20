#pragma once
#include "ParticleEffect.h"

class SmokeEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreatePuffEmitter();
	void CreateTrailEmitter();
};

