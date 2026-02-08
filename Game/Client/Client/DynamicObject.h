#pragma once
#include "GameObject.h"

class DynamicObject : public IGameObject {
public:
	virtual void PreUpdate() override;
	virtual void PostUpdate() override;
	void ResolveTerrain(OUT Vector3& outv3Delta, OUT TerrainHit& outHitResult, bool bWasGrounded);

};
