#include "pch.h"
#include "GoalMoveToPosition.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
	GoalMoveToPosition::GoalMoveToPosition(std::shared_ptr<AIAgentImpl> pOwner, const Vector3& destination)
        : Goal(pOwner, GoalType::MoveToPosition)
        , m_v3Destination(destination)
    {}

    void GoalMoveToPosition::Activate()
    {
        m_Status = Active;
        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }
        pOwner->MoveToPosition(m_v3Destination);
    }

    Goal::Status GoalMoveToPosition::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return m_Status; }

        AIPathState state = pOwner->GetPathState();

        if (state == AIPathState::PathRequested || state == AIPathState::Moving)
            return Active;

        // Idle: 경로 완료 또는 실패
        float dist = Vector3::Distance(pOwner->GetPosition(), m_v3Destination);
        m_Status = (dist < g_fArrivalThreshold) ? Completed : Failed;
        return m_Status;
    }

    void GoalMoveToPosition::Terminate()
    {
        // 별도 정리 불필요
    }
}
