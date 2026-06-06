#pragma once
#include "Scene.h"
#include "ZombiePool.h"

class GameScene : public Scene {
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

	Vector3 v3PlayerPos;

	void ProcessPlayerShoot();
	void RemoveDeadZombies();
	// 풀에서 좀비 하나를 꺼내 랜덤 NavMesh 위치에 스폰. 풀 고갈 시 아무 것도 안 함.
	void SpawnZombie();

	// 서버에서 수신한 좀비 이벤트(스폰/디스폰/상태/공격)를 매 프레임 처리.
	void ProcessNetworkZombies();
	// 서버에서 수신한 사격 결과를 소비하고 이펙트 출력.
	void ProcessShootResults();
	// 로컬 근접공격 입력 → 온라인 송신 / 오프라인 로컬 판정
	void ProcessPlayerMelee();
	// 서버 근접공격 결과 → 좀비 데미지/피 + 리모트 애니메이션
	void ProcessMeleeResults();

private:
	ZombiePool m_ZombiePool;
	//std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;

	// serverId → 클라이언트 Zombie 인스턴스 (서버 연결 시 사용)
	std::unordered_map<int, std::shared_ptr<Zombie>> m_ServerZombies;
};

//Old_Rotten_Wood_vlzhfekn_2K_Normal.dds
//Old_Concrete_Barrier_vksrdes_Mid_2K_Normal.dds
//TX_PaintedWood_A_NRM.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
