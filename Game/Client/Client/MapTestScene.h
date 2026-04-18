#pragma once
#include "Scene.h"

class TextBox;

class MapTestScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;

private:
	Vector3 v3TerrainPos;
	Vector3 v3TerrainRotation = Vector3{ 0,0,0 };

	std::shared_ptr<TextBox> m_pFPSText;
	std::shared_ptr<TextBox> m_pTimeText;
	std::shared_ptr<TextBox> m_pKoreanText;

};

