#pragma once
#include "AI.h"
#include "NavMeshImpl.h"
#include "AIAgentImpl.h"
#include "PathManager.h"
#include "AIPathPlanner.h"

namespace AIDLL
{
    class AIManagerImpl : public IAIManager
    {
    public:
        AIManagerImpl();
        virtual ~AIManagerImpl();

        // ── NavMesh ──────────────────────────────────────────────────────────
        virtual std::shared_ptr<INavMesh> GetNavMesh() override { return m_pNavMesh; }
        virtual bool LoadNavMesh(const std::string& strFileName) override;

        // ── Agent 관리 ───────────────────────────────────────────────────────
        virtual std::shared_ptr<IAIAgent> CreateAgent()  override;
        virtual void UpdateAll(float deltaTime)    override;
        virtual void SpreadAlert(const Vector3& SourcePos, int EntityId,
                                 const Vector3& TargetPos, float Radius) override;

    private:
        void UpdateFlocking(float deltaTime);
        static void GetFlockWeights(AIBehaviorState state,
                                    float& Sep, float& Ali, float& Coh);

        std::shared_ptr<NavMeshImpl>  m_pNavMesh;

        // NavMesh 로드 후 BuildGraph() 결과를 캐싱
        std::shared_ptr<NavMeshGraph> m_pNavGraph;

        // 에이전트 목록
        std::vector<std::shared_ptr<AIAgentImpl>> m_Agents;

        // 시간 분할 경로 탐색 관리자
        // 매 프레임 200 사이클을 전체 요청에 균등 배분한다.
        std::shared_ptr<PathManager> m_pPathManager;

        // ── Boids 파라미터 ──────────────────────────────────────────────────
        constexpr static float g_fSeparationRadius = 150.0f;  // cm
        constexpr static float g_fAlignmentRadius  = 400.0f;  // cm
        constexpr static float g_fCohesionRadius   = 600.0f;  // cm
        constexpr static float g_fMaxFlockForce    = 1.0f;    // 경로 방향 대비 최대 힘 비율
        constexpr static float g_fFlockSmoothing  = 4.0f;    // 지수 스무딩 속도 (Hz) — 도리도리 진동 억제
    };
}
