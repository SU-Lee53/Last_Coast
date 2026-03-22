#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalAttack (Atomic)
    // 공격 범위 내에서 Attacking 상태를 유지한다.
    // 대상이 범위를 벗어나거나 시야를 잃으면 Completed (Brain이 Chase 복귀).
    // ─────────────────────────────────────────────────────────────────────────
    class GoalAttack : public Goal
    {
    public:
        GoalAttack(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        int   m_nTargetId;
        float m_fCooldownTimer = 0.0f;  // 공격 쿨타임 누적

        static constexpr float g_fAttackRange    = 150.0f;  // 1.5m
        static constexpr float g_fAttackCooldown = 1.5f;    // 공격 간격 (초)
    };
}
