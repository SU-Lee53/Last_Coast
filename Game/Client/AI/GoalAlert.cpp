#include "pch.h"
#include "GoalAlert.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
    GoalAlert::GoalAlert(std::shared_ptr<AIAgentImpl> pOwner)
        : Goal(pOwner, GoalType::Alert)
        , m_fElapsedTime(0.0f)
    {}

    void GoalAlert::Activate()
    {
        m_Status = Active;
        m_fElapsedTime = 0.0f;

        auto pOwner = m_pOwner.lock();
        if (pOwner) 
			pOwner->CancelPath();
    }

    Goal::Status GoalAlert::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { 
			m_Status = Failed; 
			return m_Status; 
		}

        m_fElapsedTime += pOwner->GetDeltaTime();

        if (m_fElapsedTime >= g_fAlertDuration)
            m_Status = Completed;

        return m_Status;
    }

    void GoalAlert::Terminate()
    {
        // 별도 정리 불필요
    }
}
