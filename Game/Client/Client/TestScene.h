#pragma once
#include "Scene.h"
#include "NavMeshDebugRenderer.h"
#include "ZombiePool.h"

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
	// 풀에서 좀비 하나를 꺼내 랜덤 NavMesh 위치에 스폰. 풀 고갈 시 아무 것도 안 함.
	void SpawnZombie();

private:
	ZombiePool m_ZombiePool;
	std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;
};

