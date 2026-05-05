#pragma once
#include "ParticleTypes.h"

struct ParticleRenderBatch {
	TextureRef<Texture> textureRef;

	PARTICLE_BLEND_MODE eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;
	PARTICLE_SORT_MODE eSortMode = PARTICLE_SORT_MODE::BACK_TO_FRONT;

	std::vector<ParticleDrawData> drawDatas;
};

