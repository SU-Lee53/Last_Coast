#pragma once
#include "GoalComposite.h"
#include "GoalEvaluator.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalZombieThink (Composite Brain) — Raven-style Utility Goal Selection
    //
    // 매 프레임 Arbitrate()가 각 GoalEvaluator의 CalculateDesirability()를 호출해
    // 가장 높은 점수의 Goal을 SetGoal()로 활성화한다.
    //
    // 점수 범위:
    //   Attack      : 1.0        (사정거리 내 + 시야)
    //   Chase       : 0.6 ~ 0.9  (시야 내, 거리에 반비례)
    //   Investigate : 0.3 ~ 0.65 (기억 신선도에 비례)
    //   Wander      : 0.1        (기본값)
    //
    // Alert는 평가 외부에서 Wander→Investigate 전환 시 삽입된다.
    // Alert 진행 중에는 Arbitrate()가 중단된다.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalZombieThink : public GoalComposite, public std::enable_shared_from_this<GoalZombieThink>
    {
    public:
        GoalZombieThink(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

        void SetTarget(int nTargetEntityId) { m_nTargetId = nTargetEntityId; }

        // Evaluator에서 호출 — 현재 front goal이 같은 타입이면 재생성 방지
        bool notPresent(GoalType type) const;

        // Evaluator의 SetGoal()에서 호출하는 공개 전환 메서드
        void AddGoalIdle();
        void AddGoalWander();
        void AddGoalInvestigate();
        void AddGoalChase();
        void AddGoalAttack();

    private:
        void Arbitrate();
        void SwitchToGoal(GoalType type, const std::shared_ptr<AIAgentImpl>& pOwner);

        int m_nTargetId;
        GoalType m_LastSwitchedType;

        std::vector<std::shared_ptr<GoalEvaluator>> m_Evaluators;

        static constexpr float g_fAttackRange = 150.0f;
    };
}
