#pragma once
#include "GameObject.h"

class NodeObject : public IGameObject {
public:
	NodeObject() : IGameObject{ OBJECT_MOBILITY_TYPE::UNKNOWN } {}

public:
	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void PreUpdate() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

};

