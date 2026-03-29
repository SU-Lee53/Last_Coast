#pragma once
#include "GoalComposite.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalChase (Composite)
    // 시야에 있는 동안 대상을 지속적으로 추격한다.
    // 대상이 충분히 이동하면 새 Goal_MoveToPosition 으로 재계획.
    // 시야를 잃으면 Completed (Brain이 Investigate로 전환).
    // ─────────────────────────────────────────────────────────────────────────
    class GoalChase : public GoalComposite
    {
    public:
        GoalChase(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        int     m_nTargetId;
        Vector3 m_v3LastTargetPos;
        float   m_fReplanCooldown = 0.f;  // 마지막 재탐색 이후 경과 시간

        static constexpr float g_fReplanThreshold        = 40.0f;  // 재탐색 트리거 이동 거리
        static constexpr float g_fReplanCooldownDuration = 0.5f;   // 재탐색 최소 간격 (초)
    };
}
