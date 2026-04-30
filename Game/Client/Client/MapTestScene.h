#pragma once
#include "Scene.h"

class TextBox;
class TextButton;

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

	std::shared_ptr<TextBox> m_pFPSText = nullptr;
	std::shared_ptr<TextBox> m_pTimeText = nullptr;
	std::shared_ptr<TextButton> m_pKoreanText = nullptr;
	std::shared_ptr<TextBox> m_pLoadTimeText = nullptr;

	std::shared_ptr<IGameObject> m_pGun = nullptr;

	int32 m_nWeaponSelected = 0;

	nlohmann::json jWeaponData;

};

