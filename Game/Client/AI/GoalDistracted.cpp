#include "pch.h"
#include "GoalDistracted.h"
#include "AIAgentImpl.h"

namespace AIDLL
{
    GoalDistracted::GoalDistracted(std::shared_ptr<AIAgentImpl> pOwner)
        : Goal(pOwner, GoalType::Distracted)
        , m_v3Destination(Vector3::Zero)
    {}

    void GoalDistracted::Activate()
    {
        m_Status = Active;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return; }

        m_v3Destination = pOwner->GetDistractionPos();
        m_fRetryTimer   = 0.f;
        pOwner->MoveToPosition(m_v3Destination);
    }

    Goal::Status GoalDistracted::Process()
    {
        ActivateIfInactive();
        if (m_Status != Active)
            return m_Status;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) { m_Status = Failed; return m_Status; }

        // 지속시간 종료 → 완료 (다음 프레임부터 일반 Arbitrate 재개)
        if (!pOwner->HasDistraction())
        {
            m_Status = Completed;
            return m_Status;
        }

        // 유인 중 새 디코이가 터지면 목적지 갱신 (SetDistraction이 위치를 덮어씀)
        const Vector3 v3Cur = pOwner->GetDistractionPos();
        if (Vector3::Distance(v3Cur, m_v3Destination) > g_fArrivalThreshold)
        {
            m_v3Destination = v3Cur;
            m_fRetryTimer   = 0.f;
            pOwner->MoveToPosition(m_v3Destination);
            return Active;
        }

        AIPathState state = pOwner->GetPathState();
        if (state == AIPathState::PathRequested || state == AIPathState::Moving)
            return Active;

        // Idle: 도착했으면 지속시간 끝까지 제자리 대기
        const float fDist = Vector3::Distance(pOwner->GetPosition(), m_v3Destination);
        if (fDist < g_fArrivalThreshold)
            return Active;

        // 도착 전 Idle = 경로 실패/부분 경로 — 주기적으로 재요청 (매 프레임 스팸 방지)
        m_fRetryTimer += pOwner->GetDeltaTime();
        if (m_fRetryTimer >= g_fRetryInterval)
        {
            m_fRetryTimer = 0.f;
            pOwner->MoveToPosition(m_v3Destination);
        }
        return Active;
    }

    void GoalDistracted::Terminate()
    {
        // 별도 정리 불필요 — 다음 Goal이 자기 경로를 새로 설정한다
    }
}
