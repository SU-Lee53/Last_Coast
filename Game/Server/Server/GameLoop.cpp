#include "pch.h"
#include "GameLoop.h"
#include "GameWorld.h"
#include "Session.h"

namespace
{
	constexpr int   INITIAL_ZOMBIES        = 100;  // 드립 스포너가 유지할 최대 좀비 수
	constexpr int   INITIAL_SPAWN_ON_START = 50;   // 게임 시작 순간 일괄 스폰할 좀비 수
	constexpr float TICK_RATE_HZ           = 30.f; // 게임 틱 빈도
	constexpr float TICK_DT                = 1.f / TICK_RATE_HZ;
	constexpr DWORD TICK_MS                = static_cast<DWORD>(TICK_DT * 1000.f);
	constexpr DWORD ZOMBIE_SEND_INTERVAL_MS  = 100;  // 좀비 상태 정기 전송 간격
	constexpr DWORD ZOMBIE_SPAWN_INTERVAL_MS = 4000; // 좀비 스폰 간격

	// 엔딩 서바이벌(탈출 대기) 동안 스폰 폭증
	constexpr DWORD ESCAPE_SPAWN_INTERVAL_MS = 800; // 더 자주 스폰
	constexpr int   ESCAPE_MAX_ZOMBIES       = 130; // 더 많이 동시 존재
	constexpr int   ESCAPE_SPAWN_BURST       = 1;   // 스폰 1회당 마리수

	// 플레이어 사망/부활 (서버 권위)
	constexpr float PLAYER_MAX_HP          = 100.f;
	constexpr float ZOMBIE_ATTACK_DAMAGE   = 10.f;
	constexpr float PLAYER_RESPAWN_SECONDS = 10.f; // 사망 → 부활 대기 시간
}

void GameLoop::Run()
{
	timeBeginPeriod(1); // 1ms 정밀도 타이머 활성화
	DWORD dwPrevTickTime = timeGetTime();
	DWORD dwLastSpawnTime = dwPrevTickTime;
	bool  bInitialSpawnDone = false; // 게임 시작 직후 일괄 스폰 1회 완료 여부

	ZombieManager& zombies = WORLD->GetZombies();
	std::vector<ZombieStateEntry> batch; // 틱마다 재사용 (재할당 방지)

	while (WORLD->IsRunning())
	{
		DWORD dwTickStart = timeGetTime();

		// 실제 경과 시간 측정 (서버-클라이언트 시간 동기화)
		float fActualDT = (dwTickStart - dwPrevTickTime) * 0.001f;
		fActualDT = std::clamp(fActualDT, 0.001f, 0.1f); // 1ms~100ms 범위 제한
		dwPrevTickTime = dwTickStart;

		// ── 게임 시작 전(로비) — 좀비 스폰/AI/체크포인트/탈출 로직 전부 대기 ─────
		// 방장이 C2S_GAME_START로 시작해야 아래 게임 로직이 돈다.
		if (!WORLD->IsGameStarted())
		{
			dwLastSpawnTime = dwTickStart; // 시작 순간 스폰 타이머가 즉시 만료되지 않도록 계속 갱신
			DWORD dwIdleElapsed = timeGetTime() - dwTickStart;
			if (dwIdleElapsed < TICK_MS)
				Sleep(TICK_MS - dwIdleElapsed);
			continue;
		}

		// ── 게임 시작 직후 1회 — 초기 좀비 일괄 스폰 (맵 전역 스폰포인트에 분산) ───
		if (!bInitialSpawnDone)
		{
			bInitialSpawnDone = true;

			// 게임 시작 시 전원 HP/사망 상태 리셋 (재시작 대비)
			for (auto& cl : clients) {
				if (!cl.m_is_connected) continue;
				cl.m_fHP = PLAYER_MAX_HP;
				cl.m_bDead = false;
				cl.m_fRespawnTimer = 0.f;
			}

			for (int i = 0; i < INITIAL_SPAWN_ON_START; ++i)
			{
				if (zombies.GetZombies().size() >= static_cast<size_t>(INITIAL_ZOMBIES)) break;
				int nZombieId = zombies.SpawnZombie(zombies.GetRandomSpawnPoint());
				auto& zombie = zombies.GetZombies()[nZombieId];
				BroadcastAll([&](Session& cl) {
					cl.send_spawn_zombie(nZombieId, zombie.pAgent->GetPosition(), zombie.bFast);
					});
			}
			std::cout << "[Server] 게임 시작 — 초기 좀비 " << zombies.GetZombies().size() << "마리 일괄 스폰\n";
		}

		// 컷씬 중(온라인): 좀비 AI/이동/공격/스폰 정지. 클라도 정지 상태라 종료 후 스냅 없음.
		const bool bZombieFrozen = WORLD->IsZombieFrozen(dwTickStart);

		// ── 플레이어 위치 스냅샷 (clients 배열에서 직접 읽기, 세션 락으로 tearing 방지) ──
		std::unordered_map<int, Vector3> playerSnapshots;
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			if (clients[i].m_bDead) continue; // 죽은 플레이어는 좀비 타겟/체크포인트 판정에서 제외
			if (clients[i].m_is_connected) {
				std::lock_guard<std::mutex> lg(clients[i].m_state_lock);
				float x = clients[i].m_transform.m[3][0];
				float y = clients[i].m_transform.m[3][1];
				float z = clients[i].m_transform.m[3][2];
				playerSnapshots[i] = Vector3{ x, y, z };
			}
		}

		// ── 좀비 AI/이동/공격/스폰 (컷씬 정지 중에는 전체 스킵) ────────────────────
		if (!bZombieFrozen)
		{
			// ── AI Tick ──────────────────────────────────────────────────────────
			std::vector<std::pair<int, int>> attacks;                    // 데미지 발동 (Notify 딜레이 후)
			std::vector<ZombieManager::AttackAnimEvent> attackAnims;    // 공격 모션 시작 (즉시, 랜덤 인덱스 포함)
			zombies.Tick(fActualDT, playerSnapshots, attacks, attackAnims);

			// 좀비 상태 전송 — 보낼 좀비를 한 번에 모아 배치 패킷으로 전송 (WSASend/할당 횟수 대폭 감소).
			// (상태 변화 + 위치 변화 시 즉시, 아니면 100ms 간격)
			batch.clear();
			for (auto& [nId, zombie] : zombies.GetZombies())
			{
				if (!zombie.bAlive || !zombie.pAgent) continue;

				int nCurState = static_cast<int>(zombie.pAgent->GetBehaviorState());
				Vector3 v3Pos = zombie.pAgent->GetPosition();

				bool bStateChanged = (nCurState != zombie.nLastSentState);
				bool bIntervalElapsed = (dwTickStart - zombie.dwLastSendTime >= ZOMBIE_SEND_INTERVAL_MS);

				if (!bStateChanged && !bIntervalElapsed) continue;

				const auto& dbg = zombie.pAgent->GetPathDebugInfo();
				float waypointX = v3Pos.x;
				float waypointZ = v3Pos.z;
				if (!dbg.Waypoints.empty()) {
					waypointX = dbg.Waypoints.back().x;
					waypointZ = dbg.Waypoints.back().z;
				}

				ZombieStateEntry e;
				e.zombieId      = nId;
				e.x             = v3Pos.x;
				e.z             = v3Pos.z;
				e.yaw           = zombie.fYaw;
				e.waypointX     = waypointX;
				e.waypointZ     = waypointZ;
				e.behaviorState = static_cast<ZombieBehaviorState>(nCurState);
				batch.push_back(e);

				zombie.dwLastSendTime = dwTickStart;
				zombie.nLastSentState = nCurState;
			}

			// 모인 좀비를 MAX_ZOMBIE_BATCH 단위 청크로 전 클라에 전송.
			for (size_t off = 0; off < batch.size(); off += MAX_ZOMBIE_BATCH)
			{
				const int n = static_cast<int>(std::min<size_t>(MAX_ZOMBIE_BATCH, batch.size() - off));
				const ZombieStateEntry* chunk = &batch[off];
				BroadcastAll([&](Session& cl) { cl.send_zombie_state_batch(chunk, n); });
			}

			// 엔딩 서바이벌 동안에는 더 자주, 더 많이, 한 번에 여러 마리 스폰.
			const bool   bEscapeSurvival = WORLD->GetEscape().IsSurvivalActive();
			const DWORD  dwSpawnInterval = bEscapeSurvival ? ESCAPE_SPAWN_INTERVAL_MS : ZOMBIE_SPAWN_INTERVAL_MS;
			const size_t nMaxZombies     = bEscapeSurvival ? static_cast<size_t>(ESCAPE_MAX_ZOMBIES) : static_cast<size_t>(INITIAL_ZOMBIES);
			const int    nSpawnBurst     = bEscapeSurvival ? ESCAPE_SPAWN_BURST : 1;

			if (dwTickStart - dwLastSpawnTime >= dwSpawnInterval) {
				dwLastSpawnTime = dwTickStart;

				// 플레이어 위치 목록 (근처 스폰포인트 선택용 — 이미 지난 뒤쪽 포인트 제외)
				std::vector<Vector3> playerPosVec;
				playerPosVec.reserve(playerSnapshots.size());
				for (const auto& [nId, v3Pos] : playerSnapshots) playerPosVec.push_back(v3Pos);

				for (int b = 0; b < nSpawnBurst; ++b) {
					if (zombies.GetZombies().size() >= nMaxZombies) break; // 최대치 도달
					// 플레이어 근처 스폰 포인트 선택 (없으면 가장 가까운/랜덤 NavMesh 폴백)
					int nZombieId = zombies.SpawnZombie(zombies.GetSpawnPointNear(playerPosVec));
					auto& zombie = zombies.GetZombies()[nZombieId];
					BroadcastAll([&](Session& cl) {
						cl.send_spawn_zombie(nZombieId, zombie.pAgent->GetPosition(), zombie.bFast);
						});
				}
			}

			// 공격 모션 시작 (즉시 — 몽타주 재생용, 데미지 0)
			for (auto& ev : attackAnims)
			{
				BroadcastAll([&](Session& cl) {
					cl.send_zombie_attack(ev.nZombieId, ev.nTargetId, 0.f, ev.nAnimIndex);
					});
			}

			// 공격 데미지 발동 (Notify 딜레이 후 — 실제 데미지)
			// 서버가 HP를 차감하고 사망 판정까지 내린다 (서버 권위).
			for (auto& [nZombieId, nTargetId] : attacks)
			{
				if (nTargetId < 0 || nTargetId >= MAX_PLAYERS) continue;
				Session& target = clients[nTargetId];
				if (!target.m_is_connected || target.m_bDead) continue;

				BroadcastAll([&](Session& cl) {
					cl.send_zombie_attack(nZombieId, nTargetId, ZOMBIE_ATTACK_DAMAGE);
					});

				target.m_fHP = std::max(0.f, target.m_fHP - ZOMBIE_ATTACK_DAMAGE);
				if (target.m_fHP <= 0.f)
				{
					target.m_bDead = true;
					target.m_fRespawnTimer = PLAYER_RESPAWN_SECONDS;
					BroadcastAll([&](Session& cl) {
						cl.send_player_death(nTargetId, PLAYER_RESPAWN_SECONDS);
						});
					std::cout << "[Server] 플레이어 " << nTargetId << " 사망 — "
						<< PLAYER_RESPAWN_SECONDS << "초 후 부활\n";
				}
			}
		} // if (!bZombieFrozen)

		// ── 부활 타이머 (컷씬 중에도 진행) ────────────────────────────────────
		for (int i = 0; i < MAX_PLAYERS; ++i)
		{
			Session& cl = clients[i];
			if (!cl.m_is_connected || !cl.m_bDead) continue;

			cl.m_fRespawnTimer -= fActualDT;
			if (cl.m_fRespawnTimer > 0.f) continue;

			cl.m_bDead = false;
			cl.m_fHP = PLAYER_MAX_HP;
			cl.m_fRespawnTimer = 0.f;

			// 부활 위치 = 마지막 발동 체크포인트, 하나도 없으면 게임 시작 스폰 위치
			Vector3 v3Respawn;
			{
				std::lock_guard<std::mutex> lg(cl.m_state_lock);
				v3Respawn = cl.m_v3SpawnPos;
			}
			if (const Vector3* pCpPos = WORLD->GetCheckpoints().GetLastFiredPos())
				v3Respawn = *pCpPos;
			BroadcastAll([&](Session& other) {
				other.send_player_respawn(i, v3Respawn);
				});
			std::cout << "[Server] 플레이어 " << i << " 부활\n";
		}

		// ── 체크포인트 트리거: 전원이 지점 도달 시 이벤트 발사 ────────────────────
		// 진행축 +X. 모든 연결 플레이어의 min(x)가 체크포인트 x를 넘으면 1회 발사.
		if (!playerSnapshots.empty() && !WORLD->GetCheckpoints().IsEmpty())
		{
			float fMinX = playerSnapshots.begin()->second.x;
			for (const auto& [nId, v3Pos] : playerSnapshots)
				fMinX = std::min(fMinX, v3Pos.x);

			WORLD->GetCheckpoints().Update(fMinX, dwTickStart, *WORLD);
		}

		// ── 탈출 시퀀스 진행 (서버 권위) ─────────────────────────────────────
		WORLD->GetEscape().Update(dwTickStart);

		// ── 다음 틱까지 대기 ─────────────────────────────────────────────────
		DWORD dwElapsed = timeGetTime() - dwTickStart;
		if (dwElapsed < TICK_MS)
			Sleep(TICK_MS - dwElapsed);
	}

	timeEndPeriod(1);
}
