#include "pch.h"
#include "EvaluatorIdle.h"
#include "GoalZombieThink.h"

namespace AIDLL
{
    float EvaluatorIdle::CalculateDesirability(std::shared_ptr<AIAgentImpl> /*pAgent*/)
    {
        return 0.1f * m_fCharacterBias;   // 기존 Wander와 동일한 베이스라인 점수
    }

    void EvaluatorIdle::SetGoal(std::shared_ptr<GoalZombieThink> pBrain)
    {
        pBrain->AddGoalIdle();
    }
}
