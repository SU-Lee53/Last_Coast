#pragma once
#include "ParticleEffect.h"

class MuzzleFlashEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateCoreEmitter();
	void CreateShapeEmitter();
};

