#pragma once
#include "Scene.h"

class TextBox;

class LoadingScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override {}
	void OnLeaveScene() override {}
	void ProcessInput() override {}
	void Update() override;

	void BuildLights() override {}

private:
	std::shared_ptr<TextBox> m_pLoadingText = nullptr;
	float m_fDotAnimationTime = 0.f;
	uint32 m_unCurrentDots = 0;

};

