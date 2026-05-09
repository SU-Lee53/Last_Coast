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

	// 좀비를 스폰. NavMesh 위 임의 위치에 배치. 반환값: 새 좀비 ID (-1 = 실패)
	int  SpawnZombie();

	// 특정 ID 좀비를 디스폰
	void DespawnZombie(int nId);

	// 매 틱 호출 — AIManager 업데이트 + 각 좀비 Think
	// playerPositions: playerId → 월드 위치 (cm)
	// outAttacks: 이번 틱에 공격 히트가 발생한 (zombieId, targetPlayerId) 목록
	void Tick(float fDeltaTime,
	          const std::unordered_map<int, Vector3>& playerPositions,
	          std::vector<std::pair<int,int>>& outAttacks);

	// 좀비 목록 읽기 전용 접근 (틱 스레드에서 snapshot 용)
	const std::unordered_map<int, ServerZombie>& GetZombies() const { return m_Zombies; }

	std::unordered_map<int, ServerZombie>& GetZombies() { return m_Zombies; }

	std::shared_ptr<IAIManager> GetAIManager() const { return m_pAIManager; }

private:
	// FOV 없이 거리 + LOS 만으로 서버 측 가시성 판정
	bool IsVisible(const Vector3& v3From, const Vector3& v3To, float fSightRange) const;

	std::shared_ptr<IAIManager>          m_pAIManager;
	std::unordered_map<int, ServerZombie> m_Zombies;
	int                                  m_nNextId = 0;

	static constexpr float m_fSightRange   = 800.f;  // cm
	static constexpr float m_fHearingRange = 0.f;
};
