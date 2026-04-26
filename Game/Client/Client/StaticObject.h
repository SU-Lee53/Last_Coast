#pragma once
#include "GameObject.h"
class StaticObject : public IGameObject {
public:
	StaticObject() : IGameObject{ OBJECT_MOBILITY_TYPE::STATIC } {}

	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void PreUpdate() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

};

