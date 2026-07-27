#pragma once
#include "pch.h"
#include "ZombieManager.h"
#include "ServerSpatialGrid.h"
#include "CheckpointSystem.h"
#include "EscapeSequence.h"
#include "CombatSystem.h"

class Room;

// ─────────────────────────────────────────────────────────────────────────────
// ServerShared — 서버 전역 공유 자원 (SHARED 매크로로 접근).
//   방 개수와 무관하게 하나만 존재하는 것들: 서버 실행 플래그,
//   정적 OBB 공간분할(읽기 전용), 방별 GameWorld 생성에 쓰는 리소스 경로.
// ─────────────────────────────────────────────────────────────────────────────
class ServerShared {
	DECLARE_SINGLE(ServerShared)
public:
	// 정적 OBB 로드 + 방별 월드 생성용 리소스 경로 보관. main()에서 1회 호출.
	void Initialize(const std::string& navMeshPath, const std::string& spawnPointPath,
		const std::string& attackAnimPath, const std::string& sceneJsonPath,
		const std::string& modelDirectory, const std::string& checkpointPath);

	ServerSpatialGrid& GetGrid() { return m_SpatialGrid; }

	const std::string& NavMeshPath()    const { return m_navMeshPath; }
	const std::string& SpawnPointPath() const { return m_spawnPointPath; }
	const std::string& AttackAnimPath() const { return m_attackAnimPath; }
	const std::string& CheckpointPath() const { return m_checkpointPath; }

	// 서버 실행 플래그 — 게임 틱 스레드 종료 조건
	bool IsRunning() const { return m_bRunning; }
	void Stop()            { m_bRunning = false; }

private:
	ServerShared() = default;

	ServerSpatialGrid  m_SpatialGrid;   // 정적 OBB 공간 분할 (사격 차폐 판정) — 전 방 공유(읽기 전용)
	std::atomic<bool>  m_bRunning{ true };

	std::string m_navMeshPath, m_spawnPointPath, m_attackAnimPath, m_checkpointPath;
};

// ─────────────────────────────────────────────────────────────────────────────
// GameWorld — 방(Room) 하나의 게임 상태. 방마다 독립 인스턴스.
//   방장이 게임을 시작할 때 생성(PacketHandlers::GameStart)되고,
//   게임 종료(탈출 성공) 시 통째로 폐기된다(GameLoop) — 리셋 대신 재생성으로
//   좀비/체크포인트/탈출 상태 누수를 원천 차단. 같은 방에서 재시작 가능.
// ─────────────────────────────────────────────────────────────────────────────
class GameWorld {
public:
	explicit GameWorld(Room& room);

	// NavMesh/스폰포인트/공격애니/체크포인트 로드 (ServerShared 경로 사용).
	// NavMesh 실패 시 false. 방별 IAIManager가 각자 NavMesh를 로드한다.
	bool Initialize();

	Room&              GetRoom()        { return m_Room; }
	ZombieManager&     GetZombies()     { return m_ZombieManager; }
	CheckpointSystem&  GetCheckpoints() { return m_Checkpoints; }
	EscapeSequence&    GetEscape()      { return m_Escape; }
	CombatSystem&      GetCombat()      { return m_Combat; }

	// 게임 시작 여부 — 방장이 C2S_GAME_START 로 시작하기 전까지 좀비 스폰/AI/체크포인트 정지.
	// (워커 스레드가 set, 틱 스레드가 읽음)
	bool IsGameStarted() const { return m_bGameStarted; }
	void StartGame()           { m_bGameStarted = true; }

	// 전원 로딩 대기 상태 — 방장이 시작 요청 후 전원이 C2S_LOAD_COMPLETE 를 보낼 때까지 true.
	bool IsAwaitingLoads() const     { return m_bAwaitingLoads; }
	void SetAwaitingLoads(bool bSet) { m_bAwaitingLoads = bSet; }

	// 컷씬 동기화 좀비 정지 — until(timeGetTime, ms)까지 좀비 AI/이동/공격/스폰 정지. (틱 스레드 전용)
	bool IsZombieFrozen(DWORD now) const { return now < m_dwZombieFreezeUntil; }
	void ExtendZombieFreeze(DWORD until) { m_dwZombieFreezeUntil = std::max(m_dwZombieFreezeUntil, until); }

public:
	// ── 게임 틱 상태 (GameLoop 전용 — 방별로 독립) ──────────────────────────
	bool  m_bInitialSpawnDone = false; // 게임 시작 직후 일괄 스폰/HP 리셋 1회 완료 여부
	DWORD m_dwLastSpawnTime   = 0;     // 드립 스포너 타이머 (0 = 첫 틱에 초기화)

private:
	Room&              m_Room;
	ZombieManager      m_ZombieManager;
	CheckpointSystem   m_Checkpoints;
	EscapeSequence     m_Escape;
	CombatSystem       m_Combat;       // m_ZombieManager + 공유 그리드 주입 — 멤버 선언 순서 유지

	std::atomic<bool>  m_bGameStarted{ false };
	std::atomic<bool>  m_bAwaitingLoads{ false };
	DWORD              m_dwZombieFreezeUntil = 0;
};
