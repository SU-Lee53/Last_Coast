#pragma once
#include "GoalEvaluator.h"

namespace AIDLL
{
    class EvaluatorWander : public GoalEvaluator
    {
    public:
        explicit EvaluatorWander(float bias = 1.0f) : GoalEvaluator(bias) {}

        virtual float CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent) override;
        virtual void  SetGoal(std::shared_ptr<GoalZombieThink> pBrain)          override;
    };
}
