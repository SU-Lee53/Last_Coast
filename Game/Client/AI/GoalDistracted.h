#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalDistracted (Atomic)
    // 디코이 수류탄 등 강제 유인 지점(AIAgentImpl::GetDistractionPos)으로 이동해
    // 지속시간이 끝날 때까지 그 주변에 머무른다.
    // GoalZombieThink가 HasDistraction() 동안 Arbitrate를 건너뛰므로
    // 추격/공격 중이던 좀비도 어그로가 강탈된다.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalDistracted : public Goal
    {
    public:
        GoalDistracted(std::shared_ptr<AIAgentImpl> pOwner);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    private:
        Vector3 m_v3Destination;
        float   m_fRetryTimer = 0.f;   // 경로 실패 시 재요청 간격 누적

        static constexpr float g_fArrivalThreshold = 100.0f;  // 1m — 도착 판정
        static constexpr float g_fRetryInterval    = 2.0f;    // 경로 실패 재시도 주기 (초)
    };
}
