#pragma once
#include "protocol.h"

class Session;
class ZombieManager;
class ServerSpatialGrid;

// ─────────────────────────────────────────────────────────────────────────────
// 서버 권위 전투 판정. 의존성(좀비/공간분할)은 생성자 주입 — GameWorld 가 소유.
//   ResolveShoot : 사격 히트스캔 — 좀비 캡슐 + 정적 OBB 중 가까운 쪽 채택,
//                  벽 차폐(LOS) 확인 후 데미지 적용 + 결과 브로드캐스트.
//   ResolveMelee : 근접공격 — 전방 단일 레이로 가장 가까운 좀비 1마리 판정.
// ─────────────────────────────────────────────────────────────────────────────
class CombatSystem {
public:
	CombatSystem(ZombieManager& zombies, ServerSpatialGrid& grid)
		: m_Zombies(zombies), m_Grid(grid) {}

	void ResolveShoot(Session& shooter, const C2S_PlayerShoot& pkt);
	void ResolveMelee(Session& attacker, const C2S_PlayerMelee& pkt);

private:
	ZombieManager&     m_Zombies; // 좀비 레이 판정 + 데미지 적용
	ServerSpatialGrid& m_Grid;    // 정적 OBB 히트/차폐(LOS) 판정
};
