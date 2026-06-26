#include "pch.h"
#include "GoalIdle.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
    GoalIdle::GoalIdle(std::shared_ptr<AIAgentImpl> pOwner)
        : Goal(pOwner, GoalType::Idle)
    {}

    void GoalIdle::Activate()
    {
        m_Status = Active;

        auto pOwner = m_pOwner.lock();
        if (pOwner)
            pOwner->CancelPath();   // 제자리 정지
    }

    Goal::Status GoalIdle::Process()
    {
        ActivateIfInactive();
        // 제자리 대기 — brain이 다른 Goal로 전환할 때까지 Active 유지
        return m_Status;
    }

    void GoalIdle::Terminate()
    {
        // 별도 정리 불필요
    }
}
