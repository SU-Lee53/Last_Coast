#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalMoveToPosition (Atomic)
    // 지정된 월드 좌표까지 비동기 A* 경로를 요청하고 이동한다.
    // AIPathState::Idle 로 복귀 시 목적지 도달 여부로 Completed/Failed 결정.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalMoveToPosition : public Goal
    {
    public:
        GoalMoveToPosition(std::shared_ptr<AIAgentImpl> pOwner, const Vector3& destination);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        Vector3 m_v3Destination;

        static constexpr float g_fArrivalThreshold = 100.0f;  // 1m
    };
}
