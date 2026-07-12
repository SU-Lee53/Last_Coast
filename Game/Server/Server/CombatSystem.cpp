#include "pch.h"
#include "CombatSystem.h"
#include "Session.h"
#include "ZombieManager.h"
#include "ServerSpatialGrid.h"

namespace
{
	constexpr float SHOOT_MAX_DIST = 5000.f; // 사격 최대 사거리 (cm)
	constexpr float MELEE_RANGE    = 200.f;  // 근접공격 사거리 (cm)
	constexpr float MELEE_DAMAGE   = 50.f;   // 근접공격 데미지
}

void CombatSystem::ResolveShoot(Session& shooter, const C2S_PlayerShoot& pkt)
{
	Vector3 v3Origin{ pkt.originX, pkt.originY, pkt.originZ };
	Vector3 v3Dir{ pkt.dirX, pkt.dirY, pkt.dirZ };
	v3Dir.Normalize();

	const float fDamage = pkt.damage; // 무기/펠릿 데미지 (클라 전송)

	// ── 1. 좀비 히트 검사 (BoundingSphere) ──────────────────────────────
	int   nHitZombieId = -1;
	float fZombieDist  = SHOOT_MAX_DIST;
	m_Zombies.RayTestZombies(v3Origin, v3Dir, SHOOT_MAX_DIST, nHitZombieId, fZombieDist);

	// ── 2. 정적 오브젝트 히트 검사 (OBB 그리드) ─────────────────────────
	ServerRayHitResult staticHit;
	m_Grid.RayTestStatics(v3Origin, v3Dir, SHOOT_MAX_DIST, staticHit);

	// ── 3. 가장 가까운 히트 채택 ────────────────────────────────────────
	S2C_ShootResult result{};
	result.shooterPlayerId = shooter.m_id;
	result.hitZombieId = -1;
	result.damage = 0.f;
	result.bHit = 0;
	result.muzzleX  = pkt.muzzleX;
	result.muzzleY  = pkt.muzzleY;
	result.muzzleZ  = pkt.muzzleZ;
	result.shootDirX = v3Dir.x;
	result.shootDirY = v3Dir.y;
	result.shootDirZ = v3Dir.z;

	bool bZombieCloser = (nHitZombieId >= 0) &&
		(!staticHit.bHit || fZombieDist <= staticHit.fDistance);

	if (bZombieCloser)
	{
		// 좀비와 사격자 사이에 벽이 있는지 추가 차폐 체크
		Vector3 v3ZombieHitPoint = v3Origin + v3Dir * fZombieDist;
		bool bBlocked = m_Grid.IsLineOfSightBlocked(v3Origin, v3ZombieHitPoint);

		if (!bBlocked)
		{
			result.bHit = 2; // zombie
			result.hitX = v3Origin.x + v3Dir.x * fZombieDist;
			result.hitY = v3Origin.y + v3Dir.y * fZombieDist;
			result.hitZ = v3Origin.z + v3Dir.z * fZombieDist;
			result.hitNormalX = -v3Dir.x;
			result.hitNormalY = -v3Dir.y;
			result.hitNormalZ = -v3Dir.z;
			result.hitZombieId = nHitZombieId;
			result.damage = fDamage;

			// 데미지 적용 (사망 시 bAlive=false로 AI 중단, despawn은 보내지 않음)
			// 클라이언트가 TakeDamage → 사망 몽타주 → 자체 제거 처리
			m_Zombies.ApplyDamageToZombie(nHitZombieId, fDamage);
		}
		else if (staticHit.bHit)
		{
			// 벽에 차폐된 경우 → 벽 히트로 처리
			result.bHit = 1; // static
			result.hitX = staticHit.v3HitPoint.x;
			result.hitY = staticHit.v3HitPoint.y;
			result.hitZ = staticHit.v3HitPoint.z;
			result.hitNormalX = staticHit.v3HitNormal.x;
			result.hitNormalY = staticHit.v3HitNormal.y;
			result.hitNormalZ = staticHit.v3HitNormal.z;
		}
	}
	else if (staticHit.bHit)
	{
		result.bHit = 1; // static
		result.hitX = staticHit.v3HitPoint.x;
		result.hitY = staticHit.v3HitPoint.y;
		result.hitZ = staticHit.v3HitPoint.z;
		result.hitNormalX = staticHit.v3HitNormal.x;
		result.hitNormalY = staticHit.v3HitNormal.y;
		result.hitNormalZ = staticHit.v3HitNormal.z;
	}

	// ── 4. 전체 클라이언트에 결과 브로드캐스트 ──────────────────────────
	BroadcastAll([&](Session& cl) { cl.send_packet(S2C_SHOOT_RESULT, result); });
}

void CombatSystem::ResolveMelee(Session& attacker, const C2S_PlayerMelee& pkt)
{
	Vector3 v3Origin{ pkt.originX, pkt.originY, pkt.originZ };
	Vector3 v3Dir{ pkt.dirX, pkt.dirY, pkt.dirZ };
	v3Dir.Normalize();

	// 단일 레이 — 전방 사거리 내 가장 가까운 좀비 1마리
	int   nHitZombieId = -1;
	float fHitDist     = MELEE_RANGE;
	m_Zombies.RayTestZombies(v3Origin, v3Dir, MELEE_RANGE, nHitZombieId, fHitDist);

	// 근접공격 모션을 전체 클라이언트에 브로드캐스트 (리모트 애니메이션)
	BroadcastAll([&](Session& cl) { cl.send_player_melee(attacker.m_id); });

	// 히트 시 데미지 적용 + 피 이펙트 브로드캐스트
	if (nHitZombieId >= 0)
	{
		m_Zombies.ApplyDamageToZombie(nHitZombieId, MELEE_DAMAGE);
		Vector3 v3Hit = v3Origin + v3Dir * fHitDist;
		BroadcastAll([&](Session& cl) { cl.send_melee_hit(attacker.m_id, nHitZombieId, MELEE_DAMAGE, v3Hit); });
	}
}
