#pragma once
#include "AI.h"
#include "NavMeshImpl.h"
#include "AIAgentImpl.h"
#include "PathManager.h"
#include "AIPathPlanner.h"

namespace AIDLL
{
	// ── Boids 공간 격자 ──────────────────────────────────────────────────
		// 셀 크기 = Cohesion 반경(600cm): 3×3 인접 셀만 체크하면 세 반경 전부 커버
		// 매 프레임 에이전트 AABB로 범위를 동적 계산 → 맵 크기 하드코딩 불필요
	struct FlockGrid
	{
		struct Cell { std::vector<int> indices; };  // 에이전트 인덱스만 저장

		std::vector<Cell> cells;
		float fOriginX = 0.f;
		float fOriginZ = 0.f;
		float fCellSize = 1.f;
		int   nCellsX = 0;
		int   nCellsZ = 0;

		// 활성 에이전트 위치 AABB 기반으로 격자를 동적 빌드
		void Build(const std::vector<std::shared_ptr<AIAgentImpl>>& agents, float cellSize);
		void WorldToCell(float x, float z, int& cx, int& cz) const;
		int  ToIndex(int cx, int cz)  const { return cz * nCellsX + cx; }
		bool InBounds(int cx, int cz) const { return cx >= 0 && cx < nCellsX && cz >= 0 && cz < nCellsZ; }
	};


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
        virtual void SpreadDistraction(const Vector3& SourcePos, float Radius, float Duration) override;

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
        constexpr static float g_fFlockSmoothing   = 4.0f;    // 지수 스무딩 속도 (Hz) — 도리도리 진동 억제

        

        FlockGrid m_FlockGrid;
    };
}
