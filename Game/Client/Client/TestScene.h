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

	// 서버에서 수신한 좀비 이벤트(스폰/디스폰/상태/공격)를 매 프레임 처리.
	void ProcessNetworkZombies();

private:
	ZombiePool m_ZombiePool;
	std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;

	// serverId → 클라이언트 Zombie 인스턴스 (서버 연결 시 사용)
	std::unordered_map<int, std::shared_ptr<Zombie>> m_ServerZombies;
};

