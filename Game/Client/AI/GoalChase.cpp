#include "pch.h"
#include "GoalChase.h"
#include "GoalMoveToPosition.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

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

        // 대상이 충분히 이동했으면 재계획
        float moved = Vector3::Distance(rec->v3LastKnownPos, m_v3LastTargetPos);
        if (moved > g_fReplanThreshold)
        {
            m_v3LastTargetPos = rec->v3LastKnownPos;
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
