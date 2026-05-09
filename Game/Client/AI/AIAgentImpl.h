#pragma once
#include "AI.h"
#include "PathEdge.h"
#include "NavMeshGraph.h"
#include "SensoryMemory.h"

namespace AIDLL
{
    // 순환 포함 방지 전방 선언
    class AIPathPlanner;
    class NavMeshImpl;
    class PathManager;
    class GoalZombieThink;

    class AIAgentImpl : public IAIAgent, public std::enable_shared_from_this<AIAgentImpl>
    {
        friend class AIPathPlanner;  // StringPull에서 m_DebugInfo 접근 허용

    public:
        AIAgentImpl();
        virtual ~AIAgentImpl();

        // ── IAIAgent 인터페이스 ──────────────────────────────────────────────
        virtual void Update(float deltaTime) override;
        virtual void MoveToPosition(const Vector3& target) override;
        virtual void SetPosition(const Vector3& position) override;
        virtual void SyncPosition(const Vector3& position) override;
        virtual void SetMoveSpeed(float fSpeed) override { m_fMoveSpeed = fSpeed; }
        virtual Vector3 GetPosition() const override { return m_v3Position; }
        virtual AIPathState GetPathState() const override { return m_PathState; }
        virtual const PathDebugInfo& GetPathDebugInfo() const override { return m_DebugInfo; }

        virtual AIBehaviorState GetBehaviorState() const override { return m_BehaviorState; }
        virtual void UpdateSensoryStimulus(int nEntityId, const Vector3& pos,
                                           bool bVisible, bool bHeard) override;
        virtual void Think(int nTargetEntityId, float deltaTime, float fTargetDist = FLT_MAX) override;
        virtual bool ConsumeAttackHit() override;

        // ── 내부 초기화 (AIManagerImpl 에서만 호출) ──────────────────────────
        // NavMesh 설정 (구체 타입 사용)
        void SetNavMesh(std::shared_ptr<NavMeshImpl> navMesh);

        // PathPlanner 초기화 (NavMesh 로드 후 반드시 호출해야 비동기 탐색이 동작)
        void SetupPathPlanner(std::shared_ptr<NavMeshImpl> navMesh, std::shared_ptr<NavMeshGraph> navGraph, std::shared_ptr<PathManager> pathManager);

        // ── AIPathPlanner 콜백 ───────────────────────────────────────────────
        void OnPathReady (const std::list<PathEdge>& path);
        void OnPathFailed();

        // ── Goal 시스템에서 접근하는 헬퍼 ───────────────────────────────────
        SensoryMemory& GetSensoryMemory()                   { return m_SensoryMemory; }
        void           SetBehaviorState(AIBehaviorState s)  { m_BehaviorState = s; }
        float          GetDeltaTime() const                 { return m_fLastDeltaTime; }
        std::shared_ptr<NavMeshImpl> GetNavMeshInternal() const { return m_wpNavMesh.lock(); }
        void           CancelPath();  // 경로 즉시 취소 (Alert/Attack 등 이동 중단 시 호출)
        virtual void   SetDirectPath(const Vector3& target) override; // A* 없이 단일 직선 경로 설정 (서버 위치 추종 / Chase LOS용)
        void           SetAttackHitPending(bool b)          { m_bAttackHitPending = b; }

        // ── Flocking (AIManagerImpl에서만 호출) ─────────────────────────────
        void    SetFlockForce(const Vector3& force) { m_v3FlockForce = force; }
        Vector3 GetFlockForce() const               { return m_v3FlockForce; }  // 스무딩용
        Vector3 GetPathDir()  const                 { return m_v3PathDir; }  // Alignment 계산용

    private:

        std::weak_ptr<NavMeshImpl> m_wpNavMesh;    // 동기 쿼리용 (IsPointOnNavMesh 등), 구체 타입 사용
        Vector3 m_v3Position;
        AIPathState m_PathState;

        // Goal 기반 AI
        SensoryMemory                    m_SensoryMemory;
        std::shared_ptr<GoalZombieThink> m_pBrain;
        AIBehaviorState                  m_BehaviorState = AIBehaviorState::Idle;
        float                            m_fLastDeltaTime = 0.0f;

        // PathEdge 기반 경로 (비동기 완성 후 저장)
        std::vector<PathEdge> m_Path;
        int m_nPathIndex;
        float m_fMoveSpeed;

        // 공격 히트 이벤트 (GoalAttack이 쿨다운 완료 시 set, ConsumeAttackHit으로 소비)
        bool m_bAttackHitPending = false;

        // Think() 스태거링 — 시간 기반 인터벌, 프레임레이트 무관하게 일정 주기 유지
        float m_fAccumulatedDeltaTime   = 0.f; // dt 누적 타이머 겸 뇌 실행 시 전달할 경과 시간

        // 경로 추종 방향 (waypoint skip 판정 전용 — boids 힘 미포함)
        Vector3 m_v3PathDir;

        // Boids 군집 힘 (AIManagerImpl::UpdateFlocking이 매 프레임 설정)
        Vector3 m_v3FlockForce = Vector3::Zero;

        // 시간 분할 경로 탐색기 (1:1 소유, PathManager에서 weak_ptr로 참조)
        std::shared_ptr<AIPathPlanner> m_pPathPlanner;

        // 디버그 정보
        PathDebugInfo m_DebugInfo;

	private:
		constexpr static float g_fClampThreshold     = 30.f;   // 30cm 이상 벗어나면 클램핑
		constexpr static float g_fClampCheckDistance = 60.f;   // 이 거리 이상 이동 시에만 클램핑 체크 (IsPointOnNavMesh 호출 빈도 절감)

		float m_fClampAccum = 0.f;  // 마지막 클램핑 체크 이후 이동 거리 누적

		constexpr static float g_fLookAheadDist = 150.0f;  // 150cm

		// 회전 속도 제한: 5 rad/s ≈ 300°/s → 90° 전환에 약 0.3초
		constexpr static float g_fTurnSpeed = 5.0f;

		// ── 1. 이미 지나친 waypoint 건너뜀 ───────────────────────────────────
		constexpr static float g_fArrivalThreshold = 50.0f;  // 10cm → 50cm
    };
}
