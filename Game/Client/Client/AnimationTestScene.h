#pragma once
#include "Scene.h"
class AnimationTestScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList) override;

private:
	Vector3 v3TerrainPos;
	Vector3 v3TerrainRotation = Vector3{ 0,0,0 };
};

