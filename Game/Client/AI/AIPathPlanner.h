#pragma once
//-----------------------------------------------------------------------------
//
//  Name:   AIPathPlanner.h
//
//  Desc:   AIAgentImpl 에 1:1 로 대응하는 시간 분할 경로 탐색기.
//          PathManager 에 의해 관리되며, 매 프레임
//          CycleOnce() 가 호출돼 A* 탐색을 조금씩 진행한다.
//
//          Raven_PathPlanner 의 아키텍처를 3D NavMesh 에 맞게 재구현한 것.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "PathEdge.h"
#include "NavMeshGraph.h"
#include "TimeSlicedGraphAlgorithms.h"


namespace AIDLL
{
	class AIAgentImpl;
	class NavMeshImpl;
	class PathManager;

	struct Portal {
		Vector3 v3EdgeA;
		Vector3 v3EdgeB;
	};

	class AIPathPlanner : public std::enable_shared_from_this<AIPathPlanner>
	{
	public:

	private:

		enum { no_node_found = -1 };

		// ── 소유 관계 ────────────────────────────────────────────────────────
		std::weak_ptr<AIAgentImpl> m_wpOwner;  // AIAgentImpl이 shared_ptr로 소유
		std::weak_ptr<NavMeshImpl> m_wpNavMesh;
		std::weak_ptr<NavMeshGraph> m_wpNavGraph;
		std::weak_ptr<PathManager> m_wpPathManager;  // AIManagerImpl이 shared_ptr로 소유

		// ── 현재 탐색 상태 ───────────────────────────────────────────────────
		std::unique_ptr<NavMeshAStar> m_pCurrentSearch;

		Vector3 m_v3Destination;   // 요청된 목적지 월드 좌표
		int m_nStartNode;     // Dense start index
		int m_nEndNode;       // Dense end index

		// ── 내부 헬퍼 ────────────────────────────────────────────────────────

		// 위치에 해당하는 Dense 노드 인덱스 반환 (NavMesh 위 → 포함 폴리곤 우선)
		int  GetNodeForPosition(const Vector3& pos) const;

		// 진행 중인 탐색을 취소하고 PathManager 에서 등록 해제
		void GetReadyForNewSearch();

		// A* 가 반환한 폴리곤 시퀀스를 포털 교점 + LOS 단축으로 스무딩
		std::list<PathEdge> SmoothPath(const std::list<int>& polySequence, const Vector3& startPos, const Vector3& endPos) const;

	public:

		AIPathPlanner(std::shared_ptr<AIAgentImpl> owner, std::shared_ptr<NavMeshImpl> navMesh, std::shared_ptr<NavMeshGraph> graph, std::shared_ptr<PathManager> pathManager);
		~AIPathPlanner();

		// ── 경로 요청 ────────────────────────────────────────────────────────

		// 목적지까지의 비동기 A* 탐색을 시작하고 PathManager 에 등록한다.
		// 에이전트가 목적지까지 직접 이동할 수 있으면 즉시 true 반환.
		bool RequestPathToPosition(const Vector3& targetPos);

		// ── PathManager 인터페이스 ────────────────────────────────────────────

		// PathManager 가 매 프레임 호출: 탐색 한 사이클 진행
		// 반환: target_found / target_not_found / search_incomplete
		int CycleOnce();

		// ── 결과 조회 ────────────────────────────────────────────────────────

		Vector3 GetDestination() const { return m_v3Destination; }


	private:
		constexpr static float g_fEpsilon = 1.f;
	};

} // namespace AI
