#pragma once

// 서버 측 좀비 1마리의 상태
struct ServerZombie
{
	int                         nId         = -1;
	std::shared_ptr<IAIAgent>   pAgent;
	float                       fHP         = 100.f;
	bool                        bAlive      = true;

	// 마지막으로 계산된 yaw (라디안) — AIAgent 이동 방향에서 추출
	float                       fYaw        = 0.f;
	// 사망 후 제거 대기 타이머 (초). bAlive=false 후 누적 → 임계값 초과 시 제거
	float                       fDeadTimer  = 0.f;
	// 공격 애니메이션 대기 타이머 (>0이면 Think 스킵)
	float                       fAttackTimer = 0.f;
	// 공격 데미지 딜레이 (Notify 타이밍 모사). >0이면 대기 중, 0 이하 시 데미지 발동
	float                       fAttackDamageDelay = 0.f;
	int                         nAttackTargetId    = -1; // 대기 중인 공격의 타겟 플레이어 ID

	// 타겟 고정 (너무 빠른 타겟 전환 방지)
	int                         nLockedTargetId    = -1;
	float                       fTargetLockTimer   = 0.f;
	// 이전 프레임 XZ 위치 (yaw 계산용)
	Vector3                     v3PrevPos   = {};

	// ── 네트워크 전송 스로틀링 ────────────────────────────────────────────────
	DWORD                       dwLastSendTime   = 0;      // timeGetTime() 기준
	int                         nLastSentState   = -1;     // 마지막 전송한 behaviorState
};

// ─────────────────────────────────────────────────────────────────────────────
// ZombieManager
//   - 서버 부팅 시 Initialize() 호출 → AIManager 초기화 + NavMesh 로드
//   - SpawnZombie() / DespawnZombie() 로 좀비 추가/제거
//   - Tick(dt) 을 게임 틱 스레드에서 주기적으로 호출
// ─────────────────────────────────────────────────────────────────────────────
class ZombieManager
{
public:
	// NavMesh JSON 경로를 받아 AIManager 초기화
	bool Initialize(const std::string& strNavMeshPath);

	// 애니메이션 .bin 파일에서 공격 애니메이션 길이를 읽음
	bool LoadAttackAnimDuration(const std::string& strAnimBinPath);

	// 좀비를 스폰. NavMesh 위 임의 위치에 배치. 반환값: 새 좀비 ID (-1 = 실패)
	int  SpawnZombie();

	// 특정 ID 좀비를 디스폰
	void DespawnZombie(int nId);

	// 매 틱 호출 — AIManager 업데이트 + 각 좀비 Think
	// playerPositions: playerId → 월드 위치 (cm)
	// outAttacks: 이번 틱에 공격 히트가 발생한 (zombieId, targetPlayerId) 목록
	// outAttacks: 데미지 발동 (Notify 딜레이 후)
	// outAttackAnims: 공격 모션 시작 (즉시, 몽타주 재생용)
	void Tick(float fDeltaTime,
	          const std::unordered_map<int, Vector3>& playerPositions,
	          std::vector<std::pair<int,int>>& outAttacks,
	          std::vector<std::pair<int,int>>& outAttackAnims);

	// 좀비 목록 읽기 전용 접근 (틱 스레드에서 snapshot 용)
	const std::unordered_map<int, ServerZombie>& GetZombies() const { return m_Zombies; }

	std::unordered_map<int, ServerZombie>& GetZombies() { return m_Zombies; }

	std::shared_ptr<IAIManager> GetAIManager() const { return m_pAIManager; }

	// 레이 vs 좀비 BoundingSphere 교차검사. 가장 가까운 좀비를 반환.
	// outZombieId: 히트한 좀비 ID (-1 = 미히트)
	// outDist: 히트 거리
	bool RayTestZombies(const Vector3& v3Origin, const Vector3& v3Dir,
	                    float fMaxDist, int& outZombieId, float& outDist) const;

	// 좀비에게 데미지 적용. 사망 시 true 반환.
	bool ApplyDamageToZombie(int nZombieId, float fDamage);

private:
	// FOV 없이 거리 + LOS 만으로 서버 측 가시성 판정
	bool IsVisible(const Vector3& v3From, const Vector3& v3To, float fSightRange) const;

	std::shared_ptr<IAIManager>          m_pAIManager;
	std::unordered_map<int, ServerZombie> m_Zombies;
	int                                  m_nNextId = 0;

	static constexpr float m_fSightRange       = 800.f;  // cm
	static constexpr float m_fHearingRange     = 0.f;
	// 좀비 히트 캡슐 파라미터 (클라이언트 PlayerCollider 캡슐과 유사)
	static constexpr float m_fZombieCapsuleRadius     = 30.f;   // 반지름 (cm)
	static constexpr float m_fZombieCapsuleHalfHeight = 70.f;   // 반높이 (cm)
	static constexpr float m_fZombieCapsuleYOffset    = 90.f;   // NavMesh 바닥 → 캡슐 중심 Y 오프셋 (cm)
	static constexpr float m_fDeadCleanupDelay        = 3.f;    // 사망 후 서버 제거까지 대기 (초)
	static constexpr float m_fAttackDamageNotifyDelay = 1.2f;  // 공격 모션 시작 → 데미지 Notify 타이밍 (초)
	static constexpr float m_fBlendOutTime            = 0.2f;  // 몽타주 BlendOut 시간
	float                  m_fAttackAnimDuration      = 0.0f;  // 런타임에 애니메이션 파일에서 읽음
	static constexpr float m_fTargetLockDuration      = 10.f;  // 타겟 고정 시간 (초)
};
