#include "pch.h"
#include "GoalAttack.h"
#include "AIAgentImpl.h"
#include "SensoryMemory.h"

namespace AIDLL
{
    GoalAttack::GoalAttack(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId)
        : Goal(pOwner, GoalType::Attack)
        , m_nTargetId(nTargetEntityId)
    {}

    void GoalAttack::Activate()
    {
        m_Status = Active;
        m_fCooldownTimer = g_fAttackCooldown;  // 활성화 즉시 첫 공격 발동
        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }
        pOwner->CancelPath();
        pOwner->SetBehaviorState(AIBehaviorState::Attacking);
    }

    Goal::Status GoalAttack::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return m_Status; }

        auto& sm = pOwner->GetSensoryMemory();
        const SensoryRecord* rec = sm.GetRecord(m_nTargetId);

        if (!rec || !sm.CanSee(m_nTargetId))
        {
            m_Status = Completed;
            return m_Status;
        }

        float dist = Vector3::Distance(pOwner->GetPosition(), rec->v3LastKnownPos);
        if (dist > g_fAttackRange)
        {
            m_Status = Completed;  // 범위 이탈 → Chase 복귀
            return m_Status;
        }

        pOwner->SetBehaviorState(AIBehaviorState::Attacking);

        // 쿨타임 누적 → 완료 시 히트 이벤트 발행
        m_fCooldownTimer += pOwner->GetDeltaTime();
        if (m_fCooldownTimer >= g_fAttackCooldown)
        {
            m_fCooldownTimer = 0.0f;
            pOwner->SetAttackHitPending(true);
        }

        return Active;
    }

    void GoalAttack::Terminate()
    {
        // 별도 정리 불필요
    }
}
