#include "pch.h"
#include "AIManagerImpl.h"

namespace AIDLL
{
    // ─────────────────────────────────────────────────────────────────────────
    // 생성자 / 소멸자
    // ─────────────────────────────────────────────────────────────────────────
    AIManagerImpl::AIManagerImpl()
        : m_pNavMesh(std::make_shared<NavMeshImpl>())
        , m_pPathManager(std::make_shared<PathManager>(2000))   // 매 프레임 최대 2000 A* 사이클 (좀비 100마리 기준 마리당 20). 서버는 30Hz라 많은 좀비 시 예산 부족 → 상향.
    {}

    AIManagerImpl::~AIManagerImpl()
    {
        m_Agents.clear();
        m_pNavGraph.reset();
        m_pNavMesh.reset();
    }


    // ─────────────────────────────────────────────────────────────────────────
    // LoadNavMesh
    // ─────────────────────────────────────────────────────────────────────────
    bool AIManagerImpl::LoadNavMesh(const std::string& strFileName)
    {
        if (!m_pNavMesh)
            m_pNavMesh = std::make_shared<NavMeshImpl>();

        bool bResult = m_pNavMesh->LoadFromJson(strFileName);

        if (bResult)
        {
            // ── Dense 그래프 빌드 ────────────────────────────────────────────
            m_pNavGraph = m_pNavMesh->BuildGraph();

            // ── 기존 에이전트들에게 새 NavMesh / NavGraph 적용 ───────────────
            for (auto& agent : m_Agents)
            {
                agent->SetNavMesh(m_pNavMesh);

                if (m_pNavGraph && m_pPathManager)
                    agent->SetupPathPlanner(m_pNavMesh, m_pNavGraph, m_pPathManager);
            }
        }

        return bResult;
    }


    // ─────────────────────────────────────────────────────────────────────────
    // CreateAgent  : 에이전트 생성 및 현재 NavMesh / PathPlanner 할당
    // ─────────────────────────────────────────────────────────────────────────
    std::shared_ptr<IAIAgent> AIManagerImpl::CreateAgent()
    {
        auto Agent = std::make_shared<AIAgentImpl>();

        if (m_pNavMesh)
            Agent->SetNavMesh(m_pNavMesh);

        // NavGraph 가 준비된 경우 PathPlanner 도 설정
        if (m_pNavMesh && m_pNavGraph && m_pPathManager)
            Agent->SetupPathPlanner(m_pNavMesh, m_pNavGraph, m_pPathManager);

        m_Agents.push_back(Agent);
        return Agent;  // shared_ptr 직접 반환
    }


    // ─────────────────────────────────────────────────────────────────────────
    // UpdateAll  : 경로 탐색 진행 → Flocking 힘 계산 → 에이전트 이동 순으로 업데이트
    // ─────────────────────────────────────────────────────────────────────────
    void AIManagerImpl::UpdateAll(float deltaTime)
    {
        // 1) 등록된 모든 경로 탐색 요청을 사이클 예산 내에서 진행
        //    탐색 완료 시 해당 AIAgentImpl::OnPathReady() / OnPathFailed() 가 호출됨
        if (m_pPathManager)
            m_pPathManager->UpdateSearches();

        // 2) Boids 군집 힘 계산 (현재 프레임 위치 기준)
        //    agent->Update() 이전에 호출해야 이번 프레임 이동에 반영됨
        UpdateFlocking(deltaTime);

        // 3) 모든 에이전트 이동 업데이트
        for (auto& agent : m_Agents)
            agent->Update(deltaTime);
    }


    // ─────────────────────────────────────────────────────────────────────────
    // SpreadAlert  : 발견한 에이전트 주변에 청각 자극으로 경보를 전파한다.
    //               이미 직접 시야를 가진 에이전트(CanSee=true)는 건너뛴다.
    // ─────────────────────────────────────────────────────────────────────────
    void AIManagerImpl::SpreadAlert(const Vector3& SourcePos, int EntityId,
                                    const Vector3& TargetPos, float Radius)
    {
        for (auto& pAgent : m_Agents)
        {
            // 이미 직접 시야를 가진 에이전트는 건너뜀 (CanSee 상태 보호)
            if (pAgent->GetSensoryMemory().CanSee(EntityId))
                continue;

            const float fDist = Vector3::Distance(pAgent->GetPosition(), SourcePos);
            if (fDist > Radius)
                continue;

            // 청각 자극으로 경보 전달 → fTimeSinceSeen=0 설정으로 15초간 기억 유지
            pAgent->UpdateSensoryStimulus(EntityId, TargetPos, false, true);
        }
    }


    // ─────────────────────────────────────────────────────────────────────────
    // FlockGrid::Build  : 활성 에이전트 AABB 기반으로 격자를 동적 빌드
    //   - 활성 에이전트 위치의 min/max로 범위 결정 (셀 1칸 패딩)
    //   - cells 벡터의 capacity는 프레임 간 유지해 재할당 방지
    //   - 각 Cell의 indices는 clear()만 호출해 capacity 재사용
    // ─────────────────────────────────────────────────────────────────────────
    void FlockGrid::Build(
        const std::vector<std::shared_ptr<AIAgentImpl>>& agents, float cellSize)
    {
        // 1) 활성 에이전트 AABB 계산
        float fMinX =  FLT_MAX, fMinZ =  FLT_MAX;
        float fMaxX = -FLT_MAX, fMaxZ = -FLT_MAX;
        bool  bHasActive = false;

        for (const auto& pAgent : agents)
        {
            if (pAgent->GetBehaviorState() == AIBehaviorState::Idle) continue;
            const Vector3 v3Pos = pAgent->GetPosition();
            if (v3Pos.x < fMinX) fMinX = v3Pos.x;
            if (v3Pos.x > fMaxX) fMaxX = v3Pos.x;
            if (v3Pos.z < fMinZ) fMinZ = v3Pos.z;
            if (v3Pos.z > fMaxZ) fMaxZ = v3Pos.z;
            bHasActive = true;
        }

        if (!bHasActive) { nCellsX = 0; nCellsZ = 0; return; }

        // 2) 범위에 셀 1칸 패딩 → 경계 에이전트의 인접 셀 접근 보장
        fMinX -= cellSize;  fMinZ -= cellSize;
        fMaxX += cellSize;  fMaxZ += cellSize;

        fCellSize = cellSize;
        fOriginX  = fMinX;
        fOriginZ  = fMinZ;
        nCellsX   = static_cast<int>(std::ceil((fMaxX - fMinX) / cellSize));
        nCellsZ   = static_cast<int>(std::ceil((fMaxZ - fMinZ) / cellSize));

        // 3) 셀 배열 준비 — capacity 재사용, indices만 clear
        const int nTotal = nCellsX * nCellsZ;
        cells.resize(nTotal);
        for (auto& cell : cells) cell.indices.clear();

        // 4) 에이전트 인덱스를 해당 셀에 삽입
        for (int i = 0; i < static_cast<int>(agents.size()); ++i)
        {
            if (agents[i]->GetBehaviorState() == AIBehaviorState::Idle) continue;
            const Vector3 v3Pos = agents[i]->GetPosition();
            int cx, cz;
            WorldToCell(v3Pos.x, v3Pos.z, cx, cz);
            if (InBounds(cx, cz))
                cells[ToIndex(cx, cz)].indices.push_back(i);
        }
    }

    void FlockGrid::WorldToCell(float x, float z, int& cx, int& cz) const
    {
        cx = static_cast<int>(std::floor((x - fOriginX) / fCellSize));
        cz = static_cast<int>(std::floor((z - fOriginZ) / fCellSize));
    }


    // ─────────────────────────────────────────────────────────────────────────
    // UpdateFlocking  : Boids 3규칙(Separation / Alignment / Cohesion) 계산
    //                   각 에이전트의 FlockForce를 설정한다.
    // ─────────────────────────────────────────────────────────────────────────
    void AIManagerImpl::UpdateFlocking(float deltaTime)
    {
        const int n = static_cast<int>(m_Agents.size());
        if (n < 2)
            return;

        // 격자 빌드: O(n), 셀 크기 = Cohesion 반경 → 3×3 체크로 모든 반경 커버
        m_FlockGrid.Build(m_Agents, g_fCohesionRadius);
        if (m_FlockGrid.nCellsX == 0)
            return;

        for (int i = 0; i < n; ++i)
        {
            auto& AgentA  = m_Agents[i];
            AIBehaviorState StateA = AgentA->GetBehaviorState();

            // Idle 상태는 군집 행동 없음
            if (StateA == AIBehaviorState::Idle)
            {
                AgentA->SetFlockForce(Vector3::Zero);
                continue;
            }

            const Vector3 v3PosA = AgentA->GetPosition();
            const Vector3 v3DirA = AgentA->GetPathDir();

            Vector3 v3Separation = Vector3::Zero;
            Vector3 v3Alignment  = Vector3::Zero;
            Vector3 v3Cohesion   = Vector3::Zero;
            int nSepCount = 0, nAliCount = 0, nCohCount = 0;

            // 에이전트 i의 셀 좌표
            int cx, cz;
            m_FlockGrid.WorldToCell(v3PosA.x, v3PosA.z, cx, cz);

            // 3×3 인접 셀 순회 (셀 크기 = 600cm이므로 600cm 이내 에이전트 전부 포함)
            for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx)
            {
                const int nx = cx + dx;
                const int nz = cz + dz;
                if (!m_FlockGrid.InBounds(nx, nz)) continue;

                for (int j : m_FlockGrid.cells[m_FlockGrid.ToIndex(nx, nz)].indices)
                {
                    if (i == j) continue;

                    auto& AgentB = m_Agents[j];

                    Vector3 v3Diff = v3PosA - AgentB->GetPosition();
                    v3Diff.y = 0.f;
                    const float fDist = v3Diff.Length();

                    // ── Separation (단거리: 서로 밀어냄) ────────────────────
                    if (fDist < g_fSeparationRadius && fDist > 1e-4f)
                    {
                        v3Separation += v3Diff / fDist;  // 거리 반비례 가중
                        ++nSepCount;
                    }

                    // ── Alignment (중거리: 이동 방향 맞춤) ──────────────────
                    if (fDist < g_fAlignmentRadius)
                    {
                        const Vector3 v3DirB = AgentB->GetPathDir();
                        if (v3DirB.LengthSquared() > 1e-6f)
                        {
                            v3Alignment += v3DirB;
                            ++nAliCount;
                        }
                    }

                    // ── Cohesion (장거리: 그룹 중심으로 끌림) ───────────────
                    // 분리 반경의 2배를 완충 구간으로 사용:
                    //   0~150cm   → Separation만 작용 (밀어냄)
                    //   150~300cm → 아무 힘 없음 (완충)  ← 경계 진동 차단
                    //   300~600cm → Cohesion만 작용 (당김)
                    if (fDist > g_fSeparationRadius * 2.0f && fDist < g_fCohesionRadius)
                    {
                        v3Cohesion += AgentB->GetPosition();
                        ++nCohCount;
                    }
                }  // for j
            }  // for dx / for dz

            // ── 상태별 가중치 ────────────────────────────────────────────────
            float fSep, fAli, fCoh;
            GetFlockWeights(StateA, fSep, fAli, fCoh);

            Vector3 v3FlockForce = Vector3::Zero;

            if (nSepCount > 0)
            {
                float fLen = v3Separation.Length();
                if (fLen > 1e-4f) v3Separation /= fLen;  // 정규화

                // ── 정면 충돌 회피: 경로 역방향 성분 제거 ────────────────────
                // 분리력이 경로를 직접 막을 때(정면 충돌) 제자리 맴돌기가 발생한다.
                // 경로와 반대 방향 성분을 제거해 측면으로만 비켜나가게 한다.
                if (v3DirA.LengthSquared() > 1e-6f)
                {
                    float fDot = v3Separation.Dot(v3DirA);
                    if (fDot < 0.0f)
                    {
                        // 역방향 성분 제거 → 측면 성분만 남김
                        v3Separation -= v3DirA * fDot;
                        float sLen = v3Separation.Length();
                        if (sLen > 1e-4f)
                            v3Separation /= sLen;
                        else
                        {
                            // 순수 정면 충돌(측면 성분도 0) → 경로 기준 오른쪽으로 밀기
                            v3Separation = Vector3(v3DirA.z, 0.f, -v3DirA.x);
                        }
                    }
                }

                v3FlockForce += v3Separation * fSep;
            }

            if (nAliCount > 0)
            {
                v3Alignment /= static_cast<float>(nAliCount);
                float fLen = v3Alignment.Length();
                if (fLen > 1e-4f) v3Alignment /= fLen;
                v3FlockForce += v3Alignment * fAli;
            }

            if (nCohCount > 0)
            {
                Vector3 v3Center = v3Cohesion / static_cast<float>(nCohCount);
                Vector3 ToCenter = v3Center - v3PosA;
                ToCenter.y = 0.f;
                float fLen = ToCenter.Length();
                if (fLen > 1e-4f) ToCenter /= fLen;
                v3FlockForce += ToCenter * fCoh;
            }

            // 힘 크기 제한 (경로 방향을 압도하지 않도록)
            float fForceLen = v3FlockForce.Length();
            if (fForceLen > g_fMaxFlockForce)
                v3FlockForce = v3FlockForce / fForceLen * g_fMaxFlockForce;

            // ── 지수 스무딩: 이전 힘 → 새 힘으로 부드럽게 전환 ─────────────
            // 군집 상태에서 분리력이 프레임마다 방향을 반전할 때 생기는
            // 좌우 도리도리 진동을 억제한다.
            // alpha = 1 - exp(-rate * dt): rate=8 → 60fps에서 약 12%씩 블렌딩
            const float fAlpha = 1.0f - std::exp(-g_fFlockSmoothing * deltaTime);
            Vector3 v3Smoothed = Vector3::Lerp(AgentA->GetFlockForce(), v3FlockForce, fAlpha);
            AgentA->SetFlockForce(v3Smoothed);
        }
    }


    // ─────────────────────────────────────────────────────────────────────────
    // GetFlockWeights  : 행동 상태별 Boids 3규칙 가중치 반환
    // ─────────────────────────────────────────────────────────────────────────
    void AIManagerImpl::GetFlockWeights(AIBehaviorState state,
                                        float& Sep, float& Ali, float& Coh)
    {
        switch (state)
        {
        case AIBehaviorState::Wandering:
            Sep = 1.0f; Ali = 0.8f; Coh = 0.6f; break;
        case AIBehaviorState::Investigating:
            Sep = 0.8f; Ali = 0.4f; Coh = 0.5f; break;
        case AIBehaviorState::Chasing:
            Sep = 0.9f; Ali = 0.6f; Coh = 0.3f; break;
        case AIBehaviorState::Attacking:
            Sep = 1.2f; Ali = 0.0f; Coh = 0.0f; break;
        case AIBehaviorState::Alert:
            Sep = 0.6f; Ali = 0.2f; Coh = 0.4f; break;
        default:
            Sep = 0.0f; Ali = 0.0f; Coh = 0.0f; break;
        }
    }

} // namespace AI
