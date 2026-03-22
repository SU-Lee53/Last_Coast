#include "pch.h"
#include "EvaluatorInvestigate.h"
#include "GoalZombieThink.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

namespace AIDLL
{
    float EvaluatorInvestigate::CalculateDesirability(std::shared_ptr<AIAgentImpl> pAgent)
    {
        auto& sm = pAgent->GetSensoryMemory();
        if (!sm.HasMemoryOf(m_nTargetId)) return 0.f;
        if (sm.CanSee(m_nTargetId))       return 0.f;
        if (sm.IsInvestigated(m_nTargetId)) return 0.f;  // 이미 조사 완료 → 재조사 안 함

        const SensoryRecord* rec = sm.GetRecord(m_nTargetId);
        if (!rec) return 0.f;

        float freshness = 1.f - (rec->fTimeSinceSeen / SensoryMemory::g_fMemorySpan);
        return (0.3f + freshness * 0.35f) * m_fCharacterBias;
    }

    void EvaluatorInvestigate::SetGoal(std::shared_ptr<GoalZombieThink> pBrain)
    {
        pBrain->AddGoalInvestigate();
    }
}
