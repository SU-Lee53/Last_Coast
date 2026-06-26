#pragma once
#include "Scene.h"
#include "ZombiePool.h"

class TextBox;
class InputTextBox;

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
	// 오프라인 드립 스포너: 일정 간격으로 랜덤 스폰 포인트에 1마리씩 최대치까지 (기본 Idle).
	void UpdateOfflineSpawner();

	// 서버에서 수신한 좀비 이벤트(스폰/디스폰/상태/공격)를 매 프레임 처리.
	void ProcessNetworkZombies();
	// 서버에서 수신한 사격 결과를 소비하고 이펙트 출력.
	void ProcessShootResults();
	// 로컬 근접공격 입력 → 온라인 송신 / 오프라인 로컬 판정
	void ProcessPlayerMelee();
	// 서버 근접공격 결과 → 좀비 데미지/피 + 리모트 애니메이션
	void ProcessMeleeResults();
	// 서버 스크립트 게임 이벤트(폭파/포스트FX) 소비 → EventSequence에 등록
	void ProcessServerGameEvents();

	// 자체 UI 채팅 (기존 ImGui 채팅 창 대체)
	// 히스토리/입력 UI 컴포넌트 생성, Enter 폴링으로 열기/전송/닫기, 히스토리 갱신
	void BuildChatUI();
	void UpdateChat();

private:
	std::shared_ptr<class WaterGridObject> m_pWater = nullptr;
	Vector3 m_v3WaterPos{};

	ZombiePool m_ZombiePool;

	// 오프라인 드립 스폰 설정
	static constexpr int   OFFLINE_MAX_ZOMBIES   = 100;   // 동시 존재 최대 마리수
	static constexpr float OFFLINE_SPAWN_INTERVAL = 2.0f; // 스폰 간격 (초)
	float m_fOfflineSpawnTimer = 0.f;
	//std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;

	// serverId → 클라이언트 Zombie 인스턴스 (서버 연결 시 사용)
	std::unordered_map<int, std::shared_ptr<Zombie>> m_ServerZombies;

	// ── 채팅 UI ──────────────────────────────────────────────────────────────
	static constexpr int    CHAT_VISIBLE_LINES = 8;     // 화면에 표시할 히스토리 줄 수
	static constexpr size_t CHAT_MAX_HISTORY   = 100;   // 보관할 누적 라인 수

	std::shared_ptr<InputTextBox>                       m_pChatInput;
	std::array<std::shared_ptr<TextBox>, CHAT_VISIBLE_LINES> m_pChatLines{};
	std::vector<std::wstring>                           m_ChatHistory;
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
