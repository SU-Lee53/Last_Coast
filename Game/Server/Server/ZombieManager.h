#pragma once

// 서버 측 좀비 1마리의 상태
struct ServerZombie
{
	int                         nId         = -1;
	std::shared_ptr<IAIAgent>   pAgent;
	float                       fHP         = 100.f;
	bool                        bAlive      = true;
	bool                        bFast       = false; // 빠른 좀비 (스폰 시 30% 롤 — 속도/애니 구분)

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

	// Chase 상태 정지 감지용 누적 타이머
	float                       fStuckTimer      = 0.f;

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
	bool LoadAttackAnimDuration(int nAnimIndex, const std::string& strAnimBinPath);

	// 좀비를 스폰. NavMesh 위 임의 위치에 배치. 반환값: 새 좀비 ID (-1 = 실패)
	int  SpawnZombie(Vector3 SpawnPos);

	// 별도 스폰 포인트 JSON("ZombieSpawnPoints")에서 좀비 스폰 위치 목록을 로드.
	// 씬과 분리된 파일(언리얼 SaveSpawnPointsToJson 출력). 반환: 로드된 개수.
	int  LoadSpawnPoints(const std::string& strSpawnJsonPath);
	const std::vector<Vector3>& GetSpawnPoints() const { return m_SpawnPoints; }

	// 스폰 포인트 중 하나를 랜덤으로 반환. 포인트가 없으면 Vector3::Zero
	// (SpawnZombie가 Zero를 받으면 랜덤 NavMesh 위치로 폴백)
	Vector3 GetRandomSpawnPoint() const;

	// 플레이어 근처(거리 window 내) 스폰 포인트만 골라 랜덤 반환.
	// 이미 지나친 먼 뒤쪽/너무 먼 포인트를 제외해 좀비가 실제로 플레이어에게 닿게 한다.
	// window 내 후보가 없으면 가장 가까운 포인트로 폴백.
	Vector3 GetSpawnPointNear(const std::vector<Vector3>& playerPositions) const;

	// 특정 ID 좀비를 디스폰
	void DespawnZombie(int nId);

	// 공격 모션 시작 이벤트 — 어떤 공격 애니를 재생할지 인덱스 포함 (클라 몽타주 동기화)
	struct AttackAnimEvent {
		int nZombieId;
		int nTargetId;
		int nAnimIndex; // 0="Zombie Attack", 1="Zombie Attack1"
	};

	// 매 틱 호출 — AIManager 업데이트 + 각 좀비 Think
	// playerPositions: playerId → 월드 위치 (cm)
	// outAttacks: 데미지 발동 (Notify 딜레이 후) (zombieId, targetPlayerId)
	// outAttackAnims: 공격 모션 시작 (즉시, 몽타주 재생용)
	void Tick(float fDeltaTime,
	          const std::unordered_map<int, Vector3>& playerPositions,
	          std::vector<std::pair<int,int>>& outAttacks,
	          std::vector<AttackAnimEvent>& outAttackAnims);

	// 좀비 목록 읽기 전용 접근 (틱 스레드에서 snapshot 용)
	const std::unordered_map<int, ServerZombie>& GetZombies() const { return m_Zombies; }

	std::unordered_map<int, ServerZombie>& GetZombies() { return m_Zombies; }

	// 감지 반경 런타임 변경 (엔딩 서바이벌 동안 시야 무제한 등)
	void SetSightRange(float fRange) { m_fSightRange = fRange; }

	std::shared_ptr<IAIManager> GetAIManager() const { return m_pAIManager; }

	// 레이 vs 좀비 BoundingSphere 교차검사. 가장 가까운 좀비를 반환.
	// outZombieId: 히트한 좀비 ID (-1 = 미히트)
	// outDist: 히트 거리
	bool RayTestZombies(const Vector3& v3Origin, const Vector3& v3Dir,
	                    float fMaxDist, int& outZombieId, float& outDist) const;

	// 좀비에게 데미지 적용. 사망 시 true 반환.
	bool ApplyDamageToZombie(int nZombieId, float fDamage);

private:
	// 서버 측 가시성 판정: FOV 콘(전방 ±60°, 단 근거리 예외) + 거리 + LOS
	// v3Forward: 좀비 전방 단위벡터 (XZ 평면). 클라 오프라인 로직과 동일.
	bool IsVisible(const Vector3& v3From, const Vector3& v3To,
	               const Vector3& v3Forward, float fSightRange) const;

	std::shared_ptr<IAIManager>          m_pAIManager;
	std::unordered_map<int, ServerZombie> m_Zombies;
	std::vector<Vector3>                 m_SpawnPoints;   // 씬에서 로드한 고정 스폰 위치 (cm)
	int                                  m_nNextId = 0;

	float                  m_fSightRange       = 10000000.f;  // cm — FOV/LOS 없이 이 반경 내면 인지 (런타임 변경 가능)
	static constexpr float m_fHearingRange     = 0.f;
	// FOV 콘 파라미터 (클라이언트 Zombie 와 동일)
	static constexpr float m_fFOVCosHalf       = 0.5f;   // cos(60°) — 전방 ±60°
	static constexpr float m_fCloseRange       = 150.f;  // 이 거리 이내는 시야각 무시 (cm)
	// 좀비 히트 캡슐 파라미터 (클라이언트 PlayerCollider 캡슐과 유사)
	static constexpr float m_fZombieCapsuleRadius     = 30.f;   // 반지름 (cm)
	static constexpr float m_fZombieCapsuleHalfHeight = 70.f;   // 반높이 (cm)
	static constexpr float m_fZombieCapsuleYOffset    = 90.f;   // NavMesh 바닥 → 캡슐 중심 Y 오프셋 (cm)
	static constexpr float m_fDeadCleanupDelay        = 3.f;    // 사망 후 서버 제거까지 대기 (초)
	static constexpr int   ZOMBIE_ATTACK_ANIM_COUNT   = 2;     // 공격 모션 종류 수 — 클라 몽타주 섹션과 일치
	// 공격 모션 시작 → 데미지 Notify 타이밍 (초) — 모션별. 클라 몽타주 히트 notify와 동일하게 유지
	static constexpr float m_fAttackDamageNotifyDelays[ZOMBIE_ATTACK_ANIM_COUNT] = { 1.2f, 1.0f };
	static constexpr float m_fBlendOutTime            = 0.2f;  // 몽타주 BlendOut 시간
	float                  m_fAttackAnimDurations[ZOMBIE_ATTACK_ANIM_COUNT] = {};  // 런타임에 애니메이션 파일에서 읽음
	static constexpr float m_fTargetLockDuration      = 10.f;  // 타겟 고정 시간 (초)

	// 빠른 좀비 — 클라이언트 Zombie.cpp 상수와 동일하게 유지
	static constexpr int   FAST_ZOMBIE_PERCENT    = 30;    // 스폰 시 빠른 좀비 확률 (%)
	static constexpr float ZOMBIE_MOVE_SPEED      = 220.f; // 일반 좀비 (cm/s)
	static constexpr float FAST_ZOMBIE_MOVE_SPEED = 450.f; // 빠른 좀비 (cm/s)

};
