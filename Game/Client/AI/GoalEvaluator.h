#pragma once

namespace AIDLL
{
    class AIAgentImpl;
    class GoalZombieThink;

    // ─────────────────────────────────────────────────────────────────────────
    // GoalEvaluator  (Buckland 패턴 적용)
    //
    // 각 전략(Goal)의 욕구도(desirability)를 0~1 점수로 산출한다.
    // m_fCharacterBias 로 좀비마다 성격 차이를 줄 수 있다.
    //   · 1.0f = 표준
    //   · > 1.0f = 해당 전략을 더 선호
    //   · < 1.0f = 해당 전략을 덜 선호
    // ─────────────────────────────────────────────────────────────────────────
    class GoalEvaluator
    {
    public:
        explicit GoalEvaluator(float fCharacterBias) : m_fCharacterBias(fCharacterBias) {}
        virtual ~GoalEvaluator() {}

        // 욕구도 계산 (0.0 ~ 1.0)
        virtual float CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent) = 0;

        // 가장 높은 점수를 받으면 Brain에 해당 Goal을 등록
        virtual void  SetGoal(std::shared_ptr<GoalZombieThink> pBrain) = 0;

    protected:

        float m_fCharacterBias;
    };
}
