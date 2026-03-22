#include "pch.h"
#include "EvaluatorAttack.h"
#include "GoalZombieThink.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

namespace AIDLL
{

    float EvaluatorAttack::CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent)
    {
        auto& sm = pAgent->GetSensoryMemory();
        if (!sm.CanSee(m_nTargetId)) return 0.f;

        const SensoryRecord* rec = sm.GetRecord(m_nTargetId);
        if (!rec) return 0.f;

        float dist = Vector3::Distance(pAgent->GetPosition(), rec->v3LastKnownPos);
        return (dist <= g_fAttackRange) ? 1.0f * m_fCharacterBias : 0.f;
    }

    void EvaluatorAttack::SetGoal(std::shared_ptr<GoalZombieThink> pBrain)
    {
        pBrain->AddGoalAttack();
    }
}
