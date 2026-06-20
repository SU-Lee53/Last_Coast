#pragma once
#include "StaticObject.h"
class WaterGridObject : public StaticObject {
public:
	virtual void Initialize() override;
	virtual void Update() override;

	virtual void OnTraceHit(const RayTraceHitResult& hitResult) override;

};

