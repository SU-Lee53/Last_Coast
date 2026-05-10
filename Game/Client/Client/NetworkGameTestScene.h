#pragma once
#include "Scene.h"
#include "ThirdPersonPlayer.h"

class NetworkGameTestScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;
	void SyncSceneWithServer() override;

private:
	Vector3 v3TerrainPos;
	Vector3 v3TerrainRotation = Vector3{ 0,0,0 };
	
	};

