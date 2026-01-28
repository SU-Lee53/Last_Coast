#pragma once
#include "GameObject.h"
class StaticObject : public IGameObject {
	using ColliderType = StaticCollider;
public:
	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void PreUpdate() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

};

