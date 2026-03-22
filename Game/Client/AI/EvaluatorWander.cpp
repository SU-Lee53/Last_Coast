#include "pch.h"
#include "EvaluatorWander.h"
#include "GoalZombieThink.h"

namespace AIDLL
{
    float EvaluatorWander::CalculateDesirability(std::shared_ptr<AIAgentImpl> /*pAgent*/)
    {
        return 0.1f * m_fCharacterBias;
    }

    void EvaluatorWander::SetGoal(std::shared_ptr<GoalZombieThink> pBrain)
    {
        pBrain->AddGoalWander();
    }
}
