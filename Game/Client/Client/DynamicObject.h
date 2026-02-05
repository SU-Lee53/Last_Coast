#pragma once
#include "GameObject.h"

class DynamicObject : public IGameObject {
public:
	virtual void PreUpdate() override;
	virtual void PostUpdate() override;
};
