#include "pch.h"
#include "GoalInvestigate.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

namespace AIDLL
{
	GoalInvestigate::GoalInvestigate(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId)
        : Goal(pOwner, GoalType::Investigate)
        , m_nTargetId(nTargetEntityId)
        , m_v3Destination(Vector3::Zero)
    {}

    void GoalInvestigate::Activate()
    {
        m_Status = Active;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }

        const SensoryRecord* rec = pOwner->GetSensoryMemory().GetRecord(m_nTargetId);
        if (!rec) { m_Status = Failed; return; }

        m_v3Destination = rec->v3LastKnownPos;
        pOwner->MoveToPosition(m_v3Destination);
    }

    Goal::Status GoalInvestigate::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return m_Status; }

        // 기억 소진 → 조사 중단
        if (!pOwner->GetSensoryMemory().HasMemoryOf(m_nTargetId))
        {
            m_Status = Completed;
            return m_Status;
        }

        AIPathState state = pOwner->GetPathState();
        if (state == AIPathState::PathRequested || state == AIPathState::Moving)
            return Active;

        // Idle: 도착 여부 확인
        float dist = Vector3::Distance(pOwner->GetPosition(), m_v3Destination);
        if (dist < g_fArrivalThreshold)
        {
            pOwner->GetSensoryMemory().SetInvestigated(m_nTargetId);  // 조사 완료 마킹
            m_Status = Completed;
        }
        else
        {
            m_Status = Failed;
        }
        return m_Status;
    }

    void GoalInvestigate::Terminate()
    {
        // 별도 정리 불필요
    }
}
