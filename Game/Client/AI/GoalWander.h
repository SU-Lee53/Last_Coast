#pragma once
#include "GoalComposite.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // GoalWander (Composite)
    // NavMesh에서 랜덤 폴리곤 중심점을 순차적으로 방문한다.
    // Goal_MoveToPosition 완료 시 새 랜덤 포인트를 선택해 반복.
    // ─────────────────────────────────────────────────────────────────────────
    class GoalWander : public GoalComposite
    {
    public:
        explicit GoalWander(std::shared_ptr<AIAgentImpl> pOwner);

        virtual void   Activate()  override;
        virtual Status Process()   override;
        virtual void   Terminate() override;

    };
}
