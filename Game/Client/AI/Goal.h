#pragma once

namespace AIDLL
{
    class AIAgentImpl;

    enum class GoalType
    {
        Think,
        MoveToPosition,
        Idle,
        Wander,
        Alert,
        Investigate,
        Chase,
        Attack,
        Distracted
    };

    class Goal
    {
    public:
        enum Status { Active, Inactive, Completed, Failed };

    protected:
        std::weak_ptr<AIAgentImpl> m_pOwner;
        GoalType                   m_Type;
        Status                     m_Status;

        void ActivateIfInactive()
        {
            if (m_Status == Inactive)
                Activate();
        }

    public:
        Goal(std::shared_ptr<AIAgentImpl> pOwner, GoalType type)
            : m_pOwner(pOwner), m_Type(type), m_Status(Inactive)
        {}

        virtual ~Goal() {}

        virtual void   Activate()  = 0;
        virtual Status Process()   = 0;
        virtual void   Terminate() = 0;

        bool IsComplete() const { return m_Status == Completed; }
        bool IsActive()   const { return m_Status == Active;    }
        bool IsInactive() const { return m_Status == Inactive;  }
        bool HasFailed()  const { return m_Status == Failed;    }
        GoalType GetType() const { return m_Type; }
    };
}
