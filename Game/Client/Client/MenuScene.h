#pragma once
#include "Scene.h"

class InputTextBox;

class MenuScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;

private:
	bool m_bProceed = false;
};

