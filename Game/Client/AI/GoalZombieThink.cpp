#include "pch.h"
#include "GoalZombieThink.h"
#include "GoalWander.h"
#include "GoalAlert.h"
#include "GoalInvestigate.h"
#include "GoalChase.h"
#include "GoalAttack.h"
#include "EvaluatorWander.h"
#include "EvaluatorInvestigate.h"
#include "EvaluatorChase.h"
#include "EvaluatorAttack.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
    GoalZombieThink::GoalZombieThink(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId)
        : GoalComposite(pOwner, GoalType::Think)
        , m_nTargetId(nTargetEntityId)
        , m_LastSwitchedType(GoalType::Wander)
    {
        m_Evaluators.push_back(std::make_unique<EvaluatorWander>());
        m_Evaluators.push_back(std::make_unique<EvaluatorInvestigate>(m_nTargetId));
        m_Evaluators.push_back(std::make_unique<EvaluatorChase>(m_nTargetId));
        m_Evaluators.push_back(std::make_unique<EvaluatorAttack>(m_nTargetId));
    }

    void GoalZombieThink::Activate()
    {
        m_Status = Active;
		m_LastSwitchedType = GoalType::Wander;
    }

    Goal::Status GoalZombieThink::Process()
    {
        ActivateIfInactive();

        bool bSubGoalsEmpty = m_SubGoals.empty();
        GoalType curType = bSubGoalsEmpty ? m_LastSwitchedType : m_SubGoals.front()->GetType();

        // Alert 진행 중 → 타이머 완료까지 Arbitrate 중단
        if (curType == GoalType::Alert && !bSubGoalsEmpty)
            return ProcessSubgoals();

        Arbitrate();

        return ProcessSubgoals();
    }

    void GoalZombieThink::Terminate()
    {
        RemoveAllSubgoals();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Arbitrate : 각 Evaluator 점수를 계산해 최고점 Goal로 전환
    // ─────────────────────────────────────────────────────────────────────────
    void GoalZombieThink::Arbitrate()
    {
        auto pOwner = m_pOwner.lock();
        if (!pOwner) 
			return;

        float bestScore = -1.f;
        std::shared_ptr<GoalEvaluator> pBest;

        for (auto& pEval : m_Evaluators)
        {
            float score = pEval->CalculateDesirability(pOwner);
            if (score > bestScore)
            {
                bestScore = score;
                pBest = pEval;
            }
        }

        if (pBest)
            pBest->SetGoal(shared_from_this());
    }

    // ─────────────────────────────────────────────────────────────────────────
    // notPresent : front SubGoal이 이미 해당 타입이면 true (재생성 방지)
    // ─────────────────────────────────────────────────────────────────────────
    bool GoalZombieThink::notPresent(GoalType type) const
    {
        if (m_SubGoals.empty()) 
			return true;
        return m_SubGoals.front()->GetType() != type;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // AddGoal_* : Evaluator의 SetGoal()에서 호출
    // ─────────────────────────────────────────────────────────────────────────

    void GoalZombieThink::AddGoalWander()
    {
        if (notPresent(GoalType::Wander))
        {
            auto pOwner = m_pOwner.lock();
            if (!pOwner) return;
            SwitchToGoal(GoalType::Wander, pOwner);
            pOwner->SetBehaviorState(AIBehaviorState::Wandering);
        }
    }

    void GoalZombieThink::AddGoalInvestigate()
    {
        if (notPresent(GoalType::Alert) && notPresent(GoalType::Investigate))
        {
            auto pOwner = m_pOwner.lock();
            if (!pOwner) return;

            if (m_LastSwitchedType == GoalType::Wander)
            {
                // 배회 중 첫 감지 → Alert 후 Investigate
                SwitchToGoal(GoalType::Alert, pOwner);
                pOwner->SetBehaviorState(AIBehaviorState::Alert);
            }
            else if (m_LastSwitchedType == GoalType::Investigate)
            {
                // 조사 완료 후 재호출 → 목적지 도달했으므로 배회로 전환
                SwitchToGoal(GoalType::Wander, pOwner);
                pOwner->SetBehaviorState(AIBehaviorState::Wandering);
            }
            else
            {
                // Chase/Attack 후 시야 상실 → 마지막 위치 조사
                SwitchToGoal(GoalType::Investigate, pOwner);
                pOwner->SetBehaviorState(AIBehaviorState::Investigating);
            }
        }
    }

	void GoalZombieThink::AddGoalChase()
	{
		if (notPresent(GoalType::Chase))
		{
			auto pOwner = m_pOwner.lock();
			if (!pOwner) return;

			// Wander 중 첫 감지 → Alert 먼저 삽입
			if (m_LastSwitchedType == GoalType::Wander)
			{
				SwitchToGoal(GoalType::Alert, pOwner);
				pOwner->SetBehaviorState(AIBehaviorState::Alert);
			}
			else
			{
				SwitchToGoal(GoalType::Chase, pOwner);
				pOwner->SetBehaviorState(AIBehaviorState::Chasing);
			}
		}
	}

    void GoalZombieThink::AddGoalAttack()
    {
        if (notPresent(GoalType::Attack))
        {
            auto pOwner = m_pOwner.lock();
            if (!pOwner) 
				return;
            SwitchToGoal(GoalType::Attack, pOwner);
            pOwner->SetBehaviorState(AIBehaviorState::Attacking);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 헬퍼
    // ─────────────────────────────────────────────────────────────────────────

    void GoalZombieThink::SwitchToGoal(GoalType type, const std::shared_ptr<AIAgentImpl>& pOwner)
    {
        RemoveAllSubgoals();
        m_LastSwitchedType = type;

        switch (type)
        {
        case GoalType::Wander:
            AddSubgoal(std::make_unique<GoalWander>(pOwner));
            break;
        case GoalType::Alert:
            AddSubgoal(std::make_unique<GoalAlert>(pOwner));
            break;
        case GoalType::Investigate:
            AddSubgoal(std::make_unique<GoalInvestigate>(pOwner, m_nTargetId));
            break;
        case GoalType::Chase:
            AddSubgoal(std::make_unique<GoalChase>(pOwner, m_nTargetId));
            break;
        case GoalType::Attack:
            AddSubgoal(std::make_unique<GoalAttack>(pOwner, m_nTargetId));
            break;
        default:
            break;
        }
    }
}
