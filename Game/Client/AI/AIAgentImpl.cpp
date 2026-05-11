#include "pch.h"
#include "AIAgentImpl.h"
#include "AIPathPlanner.h"
#include "NavMeshImpl.h"
#include "PathManager.h"
#include "GoalZombieThink.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // 생성자 / 소멸자
    // ─────────────────────────────────────────────────────────────────────────
    AIAgentImpl::AIAgentImpl()
        : m_v3Position(Vector3::Zero)
        , m_PathState(AIPathState::Idle)
        , m_nPathIndex(0)
        , m_fMoveSpeed(300.0f)
        , m_v3PathDir(Vector3::Zero)
    {
        // 최대 LOD 인터벌(1초) 안에서 랜덤 초기 오프셋 — 1Hz 동시 폭발 방지
        m_fAccumulatedDeltaTime = static_cast<float>(rand() % 1000) / 1000.f;
    }

    AIAgentImpl::~AIAgentImpl()
    {
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SetNavMesh (내부 전용 - 구체 타입 사용)
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::SetNavMesh(std::shared_ptr<NavMeshImpl> navMesh)
    {
        m_wpNavMesh = navMesh;
    }


    // ─────────────────────────────────────────────────────────────────────────
    // SetupPathPlanner  : PathPlanner 생성/교체 (NavMesh + NavMeshGraph 로드 후 호출)
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::SetupPathPlanner(std::shared_ptr<NavMeshImpl> navMesh, std::shared_ptr<NavMeshGraph> navGraph, std::shared_ptr<PathManager> pathManager)
    {
        m_pPathPlanner.reset();
        if (navMesh && navGraph && pathManager)
        {
            m_pPathPlanner = std::make_shared<AIPathPlanner>(shared_from_this(), navMesh, navGraph, pathManager);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SetPosition  : 에이전트를 즉시 특정 위치로 이동 (경로 초기화)
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::SetPosition(const Vector3& position)
    {
        m_v3Position = position;
        m_Path.clear();
        m_nPathIndex = 0;
        m_PathState = AIPathState::Idle;
        m_v3PathDir = Vector3::Zero;
    }

	// ─────────────────────────────────────────────────────────────────────────
	// SyncPosition  : 경로/상태 초기화 없이 위치만 갱신 (충돌 해소 후 동기화용)
	// ─────────────────────────────────────────────────────────────────────────
	void AIAgentImpl::SyncPosition(const Vector3& position)
	{
		m_v3Position = position;
	}


    // ─────────────────────────────────────────────────────────────────────────
    // CancelPath  : 경로 즉시 취소 (Alert/Attack 등 이동을 중단해야 할 때 호출)
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::CancelPath()
    {
        m_Path.clear();
        m_nPathIndex = 0;
        m_PathState  = AIPathState::Idle;
        m_v3PathDir  = Vector3::Zero;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SetDirectPath  : A* 없이 현재 위치 → target 단일 엣지 경로를 즉시 설정
    //                  LOS가 열린 Chase 상태에서 A* 요청 비용을 제거한다.
    //                  이후 OnPathReady()가 호출되면 A* 결과로 덮어씌워지나,
    //                  다음 Think()에서 LOS 재확인 후 다시 SetDirectPath()로 복원된다.
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::SetDirectPath(const Vector3& target)
    {
        m_Path.clear();
        m_Path.emplace_back(m_v3Position, target);
        m_nPathIndex = 0;
        m_v3PathDir  = Vector3::Zero;
        m_PathState  = AIPathState::Moving;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MoveToPosition  : 비동기 A* 경로 요청
    // ─────────────────────────────────────────────────────────────────────────
	void AIAgentImpl::MoveToPosition(const Vector3& target)
	{
		if (m_pPathPlanner)
		{
			m_PathState = AIPathState::PathRequested;
			bool bRequested = m_pPathPlanner->RequestPathToPosition(target);
			if (!bRequested)
				m_PathState = AIPathState::Idle;
		}
		else
		{
			m_PathState = AIPathState::Idle;
		}
	}


    // ─────────────────────────────────────────────────────────────────────────
    // OnPathReady  : AIPathPlanner 가 경로 완성 시 호출하는 콜백
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::OnPathReady(const std::list<PathEdge>& path)
    {
        m_Path.assign(path.begin(), path.end());
        m_nPathIndex = 0;
        m_v3PathDir = Vector3::Zero;  // 새 경로 시작 시 방향 리셋 (반대방향 웨이포인트 skip 방지)
        m_PathState = (!m_Path.empty()) ? AIPathState::Moving : AIPathState::Idle;
    }


    // ─────────────────────────────────────────────────────────────────────────
    // OnPathFailed  : AIPathPlanner 가 경로 탐색 실패 시 호출하는 콜백
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::OnPathFailed()
    {
        m_Path.clear();
        m_nPathIndex = 0;
        m_PathState = AIPathState::Idle;
    }


    // ─────────────────────────────────────────────────────────────────────────
    // Update  : 경로를 따라 이동 처리 (매 프레임 호출)
    // ─────────────────────────────────────────────────────────────────────────
	void AIAgentImpl::Update(float deltaTime)
	{
		// Attack/Alert 상태에서는 이동 처리 없이 즉시 반환
		if (m_BehaviorState == AIBehaviorState::Attacking ||
			m_BehaviorState == AIBehaviorState::Alert)
			return;

		if (m_PathState == AIPathState::PathRequested)
			return;
		if (m_PathState != AIPathState::Moving || m_Path.empty())
			return;

		// 2. waypoint 건너뜀 조건을 거리뿐 아니라 "지나쳤는지"도 체크
		while (m_nPathIndex < static_cast<int>(m_Path.size()))
		{
			const Vector3 v3WP = m_Path[m_nPathIndex].Destination();
			Vector3 ToWP = v3WP - m_v3Position;
			ToWP.y = 0.0f;

			if (ToWP.Length() < g_fArrivalThreshold)
			{
				m_nPathIndex++;
				continue;
			}

			// 진행방향과 waypoint 방향이 반대면 (이미 지나침) 건너뜀
			if (m_v3PathDir.LengthSquared() > 1e-6f)
			{
				ToWP.Normalize();
				float dot = m_v3PathDir.Dot(ToWP);
				if (dot < 0.0f)  // 180도 반대방향 → 이미 지나침
				{
					m_nPathIndex++;
					continue;
				}
			}
			break;
		}

		// 모든 웨이포인트 소진 → 경로 완료
		if (m_nPathIndex >= static_cast<int>(m_Path.size()))
		{
			m_PathState  = AIPathState::Idle;
			m_v3PathDir  = Vector3::Zero;
			return;
		}

		// ── 2. 경로 위 look-ahead 점 계산 ────────────────────────────────────
		//  현재 위치에서 경로를 따라 kLookAheadDist만큼 앞에 있는 점을 목표로 삼는다.


		Vector3 v3LookAheadPoint = m_v3Position;
		float fRemaining = g_fLookAheadDist;
		int nIdx = m_nPathIndex;

		while (nIdx < static_cast<int>(m_Path.size()) && fRemaining > 0.0f)
		{
			const Vector3 v3Dest = m_Path[nIdx].Destination();
			Vector3 ToNext = v3Dest - v3LookAheadPoint;
			ToNext.y = 0.0f;
			float dist = ToNext.Length();

			if (dist <= fRemaining)
			{
				v3LookAheadPoint = v3Dest;
				fRemaining -= dist;
				nIdx++;
			}
			else
			{
				ToNext.Normalize();
				v3LookAheadPoint += ToNext * fRemaining;
				fRemaining = 0.0f;
			}
		}

		// ── 3. look-ahead 방향으로 이동 (방향 스무딩으로 스피닝 방지) ─────────
		//  교점 통과 시 look-ahead 방향이 급변해도 회전 속도를 제한해
		//  자연스럽게 방향을 전환한다.
		Vector3 v3TargetDir = v3LookAheadPoint - m_v3Position;
		v3TargetDir.y = 0.0f;

		if (v3TargetDir.LengthSquared() > 1e-6f)
		{
			v3TargetDir.Normalize();

			if (m_v3PathDir.LengthSquared() < 1e-6f)
			{
				// 첫 이동이거나 정지 직후 → 즉시 방향 설정
				m_v3PathDir = v3TargetDir;
			}
			else
			{
				float fBlend = std::min(1.0f, g_fTurnSpeed * deltaTime);
				m_v3PathDir = Vector3::Lerp(m_v3PathDir, v3TargetDir, fBlend);

				float fLen = m_v3PathDir.Length();
				if (fLen > 1e-4f)
					m_v3PathDir /= fLen;
				else
					m_v3PathDir = v3TargetDir;
			}

			// ── Boids 힘 블렌딩 ────────────────────────────────────────────
			// m_v3PathDir: waypoint skip 판정에만 사용 (순수 경로 방향, 변경 없음)
			// v3FinalDir: 실제 이동에 사용 (경로 방향 + flock 힘 합산 후 정규화)
			Vector3 v3FinalDir = m_v3PathDir + m_v3FlockForce;
			v3FinalDir.y = 0.f;
			float fFinalLen = v3FinalDir.Length();
			if (fFinalLen > 1e-4f)
				v3FinalDir /= fFinalLen;
			else
				v3FinalDir = m_v3PathDir;

			m_v3Position += v3FinalDir * (m_fMoveSpeed * deltaTime);
		}

		// ── 4. NavMesh 클램핑 ─────────────────────────────────────────────────
		// IsPointOnNavMesh()는 모든 타일/폴리곤 순회 → 매 프레임 실행 시 비용 큼
		// 이동 거리를 누적해 g_fClampCheckDistance 이상 움직였을 때만 체크
		m_fClampAccum += m_fMoveSpeed * deltaTime;
		if (m_fClampAccum >= g_fClampCheckDistance)
		{
			m_fClampAccum = 0.f;
			if (auto navMesh = m_wpNavMesh.lock())
			{
				if (!navMesh->IsPointOnNavMesh(m_v3Position))
				{
					Vector3 v3Nearest = navMesh->GetNearestPointOnNavMesh(m_v3Position);
					if (Vector3::Distance(m_v3Position, v3Nearest) > g_fClampThreshold)
						m_v3Position = v3Nearest;
				}
			}
		}
	}
    // ─────────────────────────────────────────────────────────────────────────
    // UpdateSensoryStimulus  : 현재 프레임의 감각 자극 기록
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::UpdateSensoryStimulus(int EntityId, const Vector3& pos,
                                            bool Visible, bool Heard)
    {
        m_SensoryMemory.UpdateStimulus(EntityId, pos, Visible, Heard);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Think  : SensoryMemory를 감쇠하고 Goal Brain을 구동한다.
    //          Zombie::PostUpdate에서 UpdateSensoryStimulus 이후 호출.
    // ─────────────────────────────────────────────────────────────────────────
    void AIAgentImpl::Think(int nTargetEntityId, float deltaTime, float fTargetDist)
    {
        // 스킵된 프레임 dt 누적 — 뇌가 실행될 때 한 번에 전달해 Goal 타이머 정확도 유지
        m_fAccumulatedDeltaTime += deltaTime;

        // 감지되지 않은 엔티티의 기억 감퇴 — 감쇠 정확도를 위해 매 프레임 실행
        m_SensoryMemory.Update(deltaTime);

        // 브레인 초기화 (첫 호출 시)
        if (!m_pBrain)
        {
            m_pBrain = std::make_shared<GoalZombieThink>(shared_from_this(), nTargetEntityId);
            m_BehaviorState = AIBehaviorState::Wandering;
        }
        else
        {
            m_pBrain->SetTarget(nTargetEntityId);
        }

        // ── AI LOD: 타겟까지 거리에 따라 Think 주기 조정 ──────────────────────
        // 시야 범위(800cm) 바깥은 낮은 주기로도 충분 — 원거리 좀비 CPU 비용 절감
        float fEffectiveInterval;
        if      (fTargetDist < 1000.f) fEffectiveInterval = 0.05f;  // ≤10m:  20Hz (전투/감지 범위)
        else if (fTargetDist < 3000.f) fEffectiveInterval = 0.2f;   // ≤30m:  5Hz  (중거리)
        else                           fEffectiveInterval = 1.0f;   //  >30m:  1Hz  (원거리)

        if (m_fAccumulatedDeltaTime >= fEffectiveInterval)
        {
            m_fLastDeltaTime        = m_fAccumulatedDeltaTime;
            m_fAccumulatedDeltaTime -= fEffectiveInterval; // 초과분 이월 — 다음 사이클에도 위상 분산 유지
            m_pBrain->Process();
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ConsumeAttackHit : GoalAttack이 발행한 히트 이벤트를 소비 (1회성)
    // ─────────────────────────────────────────────────────────────────────────
    bool AIAgentImpl::ConsumeAttackHit()
    {
        if (m_bAttackHitPending)
        {
            m_bAttackHitPending = false;
            return true;
        }
        return false;
    }

} // namespace AI
