#pragma once
#include "GoalEvaluator.h"

namespace AIDLL
{
    // 기본(베이스라인) 평가자 — 다른 행동 점수가 모두 낮으면 Idle 유지
    class EvaluatorIdle : public GoalEvaluator
    {
    public:
        explicit EvaluatorIdle(float bias = 1.0f) : GoalEvaluator(bias) {}

        virtual float CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent) override;
        virtual void  SetGoal(std::shared_ptr<GoalZombieThink> pBrain)          override;
    };
}
