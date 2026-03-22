#include "pch.h"
#include "GoalWander.h"
#include "GoalMoveToPosition.h"
#include "AIAgentImpl.h"
#include "NavMeshImpl.h"

namespace AIDLL
{
    GoalWander::GoalWander(std::shared_ptr<AIAgentImpl> pOwner)
        : GoalComposite(pOwner, GoalType::Wander)
    {}

    void GoalWander::Activate()
    {
        m_Status = Active;
        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }

        auto navMesh = pOwner->GetNavMeshInternal();
        Vector3 target = navMesh ? navMesh->GetRandomPoint() : pOwner->GetPosition();
        AddSubgoal(std::make_unique<GoalMoveToPosition>(pOwner, target));
    }

    Goal::Status GoalWander::Process()
    {
        ActivateIfInactive();

        Status s = ProcessSubgoals();

        // 도착하거나 실패하면 새 랜덤 목표 선택
        if (s == Completed || s == Failed)
        {
            auto pOwner = m_pOwner.lock();
            if (!pOwner) return Failed;

            auto navMesh = pOwner->GetNavMeshInternal();
            Vector3 target = navMesh ? navMesh->GetRandomPoint() : pOwner->GetPosition();
            AddSubgoal(std::make_unique<GoalMoveToPosition>(pOwner, target));
            return Active;
        }
        return s;
    }

    void GoalWander::Terminate()
    {
        RemoveAllSubgoals();
    }
}
