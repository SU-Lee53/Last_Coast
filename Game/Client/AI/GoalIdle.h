#pragma once
#include "Goal.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalIdle (Atomic)
    // 제자리 대기 상태. 활성화 시 경로를 취소해 정지하고 이후 아무 것도 하지 않는다.
    // 타겟을 감지하면 brain의 Arbitrate가 Chase/Attack 등으로 전환한다.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalIdle : public Goal
    {
    public:
        explicit GoalIdle(std::shared_ptr<AIAgentImpl> pOwner);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;
    };
}
