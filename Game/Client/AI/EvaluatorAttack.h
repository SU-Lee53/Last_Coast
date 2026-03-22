#pragma once
#include "GoalEvaluator.h"

namespace AIDLL
{
    class EvaluatorAttack : public GoalEvaluator
    {
    public:
        EvaluatorAttack(int nTargetId, float bias = 1.0f) : GoalEvaluator(bias), m_nTargetId(nTargetId) {}

        virtual float CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent) override;
        virtual void  SetGoal(std::shared_ptr<GoalZombieThink> pBrain)          override;

    private:
        int m_nTargetId;

		static constexpr float g_fAttackRange = 150.0f;
    };
}
