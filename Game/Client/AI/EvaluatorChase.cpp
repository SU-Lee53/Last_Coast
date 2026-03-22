#include "pch.h"
#include "EvaluatorChase.h"
#include "GoalZombieThink.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

namespace AIDLL
{
    static constexpr float g_fAttackRange = 150.0f;
    static constexpr float g_fSightRange  = 800.0f;

    float EvaluatorChase::CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent)
    {
        auto& sm = pAgent->GetSensoryMemory();
        if (!sm.CanSee(m_nTargetId)) return 0.f;

        const SensoryRecord* rec = sm.GetRecord(m_nTargetId);
        if (!rec) return 0.f;

        float dist = Vector3::Distance(pAgent->GetPosition(), rec->v3LastKnownPos);
        if (dist <= g_fAttackRange) 
			return 0.f;  // Attack이 담당

        float t = 1.f - std::clamp(dist / g_fSightRange, 0.f, 1.f);
        return (0.6f + t * 0.3f) * m_fCharacterBias;
    }

    void EvaluatorChase::SetGoal(std::shared_ptr<GoalZombieThink> pBrain)
    {
        pBrain->AddGoalChase();
    }
}
