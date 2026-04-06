#pragma once
#include "Scene.h"
#include "NavMeshDebugRenderer.h"

class TestScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;
	
private:
	void ProcessPlayerShoot();
	void RemoveDeadZombies();

private:
	std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;
};

