#pragma once
#include "Goal.h"

namespace AIDLL
{
    class GoalComposite : public Goal
    {
    protected:
        std::list<std::unique_ptr<Goal>> m_SubGoals;

        void AddSubgoal(std::unique_ptr<Goal> g)
        {
            m_SubGoals.push_front(std::move(g));
        }

        void RemoveAllSubgoals()
        {
            for (auto& g : m_SubGoals)
                g->Terminate();
            m_SubGoals.clear();
        }

        Status ProcessSubgoals()
        {
            while (!m_SubGoals.empty() &&
                   (m_SubGoals.front()->IsComplete() || m_SubGoals.front()->HasFailed()))
            {
                m_SubGoals.front()->Terminate();
                m_SubGoals.pop_front();
            }

            if (!m_SubGoals.empty())
            {
                Status s = m_SubGoals.front()->Process();
                if (s == Completed && m_SubGoals.size() > 1)
                    return Active;
                return s;
            }
            return Completed;
        }

    public:
        GoalComposite(std::shared_ptr<AIAgentImpl> pOwner, GoalType type)
            : Goal(pOwner, type)
        {}

        virtual ~GoalComposite()
        {
            RemoveAllSubgoals();
        }
    };
}
