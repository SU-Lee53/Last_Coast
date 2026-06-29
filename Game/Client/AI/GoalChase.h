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

        // 재탐색 빈도 — 낮을수록 정확하지만 A* 요청 폭주로 서버(다수 좀비)에서 예산 고갈 → "오다 멈춤".
        // 키워서 요청을 줄이면 각 경로가 더 빨리 완성돼 끊김이 준다(약간 stale한 추격은 스왈에선 무방).
        static constexpr float g_fReplanThreshold        = 150.0f;  // 재탐색 트리거 이동 거리 (40→150)
        static constexpr float g_fReplanCooldownDuration = 1.0f;    // 재탐색 최소 간격 초 (0.5→1.0)
    };
}
