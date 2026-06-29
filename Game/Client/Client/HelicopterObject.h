#pragma once
#include "DynamicObject.h"

class HelicopterObject : public DynamicObject {
public:
	void Initialize() override;
	void ProcessInput() override;
	void Update() override;

protected:
	std::shared_ptr<IGameObject> m_pMainRotorFrame = nullptr;

};

