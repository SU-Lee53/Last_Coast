#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalInvestigate (Atomic)
    // SensoryMemory의 마지막 지각 위치로 이동한다.
    // 도착하거나 기억이 소진되면 완료.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalInvestigate : public Goal
    {
    public:
        GoalInvestigate(std::shared_ptr<AIAgentImpl> pOwner, int nTargetEntityId);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        int     m_nTargetId;
        Vector3 m_v3Destination;

        static constexpr float g_fArrivalThreshold = 100.0f;  // 1m
    };
}
