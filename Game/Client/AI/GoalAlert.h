#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalAlert (Atomic)
    // 1.5초 동안 이동을 멈추고 Alert 상태를 유지한다.
    // 완료 시 GoalZombieThink 가 Investigate 로 전환한다.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalAlert : public Goal
    {
    public:
        explicit GoalAlert(std::shared_ptr<AIAgentImpl> pOwner);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        float m_fElapsedTime = 0.0f;

        static constexpr float g_fAlertDuration = 1.5f;
    };
}
