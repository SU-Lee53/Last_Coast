#pragma once
#include "ParticleEffect.h"
class BloodEffect : public IParticleEffect {
public:
	virtual void Initialize() override;

private:
	void CreateMistEmitter();
	void CreateDropEmitter();
	void CreateSplatterEmitter();
};

