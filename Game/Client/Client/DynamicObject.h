#pragma once
#include "GameObject.h"

class DynamicObject : public IGameObject {
public:
	DynamicObject() : IGameObject{ OBJECT_MOBILITY_TYPE::DYNAMIC } {}
	virtual ~DynamicObject() {}

	virtual void PreUpdate() override;
	virtual void PostUpdate() override;
	virtual void PostAnimationUpdate() override;
	void ResolveTerrain(OUT Vector3& outv3Delta, OUT TerrainHit& outHitResult, bool bWasGrounded);

};
