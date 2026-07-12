#pragma once
#include "pch.h"
#include "ZombieManager.h"
#include "ServerSpatialGrid.h"
#include "CheckpointSystem.h"
#include "EscapeSequence.h"
#include "CombatSystem.h"
#include "PacketHandlers.h"

// ─────────────────────────────────────────────────────────────────────────────
// 게임 상태의 단일 소유자.
//   main.cpp 에 흩어져 있던 전역(좀비/공간분할/체크포인트/탈출/시작 플래그)을
//   묶고, 하위 시스템(CombatSystem/PacketHandlers)에 의존성을 주입한다.
//   접근은 WORLD 매크로(GameWorld::GetInstance()) 한 곳으로 — extern 선언 금지.
// ─────────────────────────────────────────────────────────────────────────────
class GameWorld {
	DECLARE_SINGLE(GameWorld)
public:
	// NavMesh/스폰포인트/공격애니/정적OBB/체크포인트 로드. NavMesh 실패 시 false.
	bool Initialize(const std::string& navMeshPath, const std::string& spawnPointPath, const std::string& attackAnimPath,
		const std::string& sceneJsonPath, const std::string& modelDirectory, const std::string& checkpointPath);

	ZombieManager&     GetZombies()     { return m_ZombieManager; }
	ServerSpatialGrid& GetGrid()        { return m_SpatialGrid; }
	CheckpointSystem&  GetCheckpoints() { return m_Checkpoints; }
	EscapeSequence&    GetEscape()      { return m_Escape; }
	CombatSystem&      GetCombat()      { return m_Combat; }
	PacketHandlers&    GetHandlers()    { return m_Handlers; }

	// 서버 실행 플래그 — 게임 틱 스레드 종료 조건
	bool IsRunning() const { return m_bRunning; }
	void Stop()            { m_bRunning = false; }

	// 게임 시작 여부 — 방장이 C2S_GAME_START 로 시작하기 전까지 좀비 스폰/AI/체크포인트 정지.
	// (워커 스레드가 set, 틱 스레드가 읽음)
	bool IsGameStarted() const { return m_bGameStarted; }
	void StartGame()           { m_bGameStarted = true; }

	// 컷씬 동기화 좀비 정지 — until(timeGetTime, ms)까지 좀비 AI/이동/공격/스폰 정지.
	// 클라 컷씬과 동기화해 종료 후 스냅 방지. (틱 스레드 전용)
	bool IsZombieFrozen(DWORD now) const { return now < m_dwZombieFreezeUntil; }
	void ExtendZombieFreeze(DWORD until) { m_dwZombieFreezeUntil = std::max(m_dwZombieFreezeUntil, until); }

private:
	GameWorld() : m_Combat(m_ZombieManager, m_SpatialGrid), m_Handlers(*this) {}

	ZombieManager      m_ZombieManager;
	ServerSpatialGrid  m_SpatialGrid;          // 정적 OBB 공간 분할 (사격 차폐 판정)
	CheckpointSystem   m_Checkpoints;
	EscapeSequence     m_Escape;
	CombatSystem       m_Combat;               // m_ZombieManager/m_SpatialGrid 주입 — 멤버 선언 순서 유지
	PacketHandlers     m_Handlers;

	std::atomic<bool>  m_bGameStarted{ false };
	std::atomic<bool>  m_bRunning{ true };
	DWORD              m_dwZombieFreezeUntil = 0;
};
