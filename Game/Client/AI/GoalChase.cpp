#include "pch.h"
#include "GoalChase.h"
#include "GoalMoveToPosition.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"
#include "NavMeshImpl.h"

namespace AIDLL
{
	GoalChase::GoalChase(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId)
        : GoalComposite(pOwner, GoalType::Chase)
        , m_nTargetId(nTargetEntityId)
        , m_v3LastTargetPos(Vector3::Zero)
    {}

    void GoalChase::Activate()
    {
        m_Status = Active;
        m_fReplanCooldown = g_fReplanCooldownDuration;  // 활성화 즉시 경로 탐색 → 쿨다운 시작

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }

        const SensoryRecord* rec = pOwner->GetSensoryMemory().GetRecord(m_nTargetId);
        if (rec)
        {
            m_v3LastTargetPos = rec->v3LastKnownPos;
            AddSubgoal(std::make_unique<GoalMoveToPosition>(pOwner, m_v3LastTargetPos));
        }
        else
        {
            m_Status = Completed;
        }
    }

    Goal::Status GoalChase::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return m_Status; }

        auto& sm = pOwner->GetSensoryMemory();

        // 시야를 잃으면 완료 → Brain이 Investigate 전환
        if (!sm.CanSee(m_nTargetId))
        {
            m_Status = Completed;
            return m_Status;
        }

        const SensoryRecord* rec = sm.GetRecord(m_nTargetId);
        if (!rec)
        {
            m_Status = Completed;
            return m_Status;
        }

        const Vector3 v3TargetPos = rec->v3LastKnownPos;
        auto pNavMesh = pOwner->GetNavMeshInternal();

        // ── LOS 열림: A* 없이 직선 경로로 직접 추격 ─────────────────────────
        if (pNavMesh && pNavMesh->IsLineOfSightClear(pOwner->GetPosition(), v3TargetPos))
        {
            RemoveAllSubgoals();                   // 진행 중인 A* 서브골 제거
            pOwner->SetDirectPath(v3TargetPos);    // 단일 엣지 경로 즉시 설정
            m_v3LastTargetPos = v3TargetPos;
            m_fReplanCooldown = 0.f;               // LOS가 막히는 순간 즉시 A* 재탐색 허용
            return Active;
        }

        // ── LOS 막힘: A* 재탐색 (거리 조건 + 쿨다운) ────────────────────────
        m_fReplanCooldown += pOwner->GetDeltaTime();
        float fMoved = Vector3::Distance(v3TargetPos, m_v3LastTargetPos);

        // 서브골이 없으면 LOS 직접 추격에서 막 전환된 상태 → 쿨다운/거리 무시하고 즉시 재탐색
        // (이 조건이 없으면 ProcessSubgoals()가 빈 리스트에서 Completed를 반환해 Chase가 즉시 종료됨)
        const bool bNeedsReplan = m_SubGoals.empty() ||
            (fMoved > g_fReplanThreshold && m_fReplanCooldown >= g_fReplanCooldownDuration);

        if (bNeedsReplan)
        {
            m_v3LastTargetPos = v3TargetPos;
            m_fReplanCooldown = 0.f;
            RemoveAllSubgoals();
            AddSubgoal(std::make_unique<GoalMoveToPosition>(pOwner, m_v3LastTargetPos));
        }

        return ProcessSubgoals();
    }

    void GoalChase::Terminate()
    {
        RemoveAllSubgoals();
    }
}
