#include "pch.h"
#include "AIPathPlanner.h"
#include "AIAgentImpl.h"
#include "NavMeshImpl.h"
#include "PathManager.h"

namespace AIDLL
{
	// ─────────────────────────────────────────────────────────────────────────
	// 생성자 / 소멸자
	// ─────────────────────────────────────────────────────────────────────────
	AIPathPlanner::AIPathPlanner(std::shared_ptr<AIAgentImpl> owner, std::shared_ptr<NavMeshImpl> navMesh, std::shared_ptr<NavMeshGraph> graph, std::shared_ptr<PathManager> pathManager)
		: m_wpOwner(owner)
		, m_wpNavMesh(navMesh)
		, m_wpNavGraph(graph)
		, m_wpPathManager(pathManager)
		, m_pCurrentSearch(nullptr)
		, m_v3Destination(Vector3::Zero)
		, m_nStartNode(no_node_found)
		, m_nEndNode(no_node_found)
	{
	}

	AIPathPlanner::~AIPathPlanner()
	{
		GetReadyForNewSearch();
	}


	// ─────────────────────────────────────────────────────────────────────────
	// GetReadyForNewSearch  : 진행 중인 탐색 취소 및 PathManager 등록 해제
	// ─────────────────────────────────────────────────────────────────────────
	void AIPathPlanner::GetReadyForNewSearch()
	{
		if (auto pathManager = m_wpPathManager.lock())
			pathManager->UnRegister(this);

		m_pCurrentSearch.reset();
	}


	// ─────────────────────────────────────────────────────────────────────────
	// GetNodeForPosition  : 위치에 해당하는 Dense 노드 인덱스 반환
	// ─────────────────────────────────────────────────────────────────────────
	int AIPathPlanner::GetNodeForPosition(const Vector3& pos) const
	{
		auto navMesh = m_wpNavMesh.lock();
		auto navGraph = m_wpNavGraph.lock();
		if (!navMesh || !navGraph)
			return no_node_found;

		// 1. 정확히 포함된 폴리곤 탐색
		int nGlobalPolyID = navMesh->FindPolygonContaining(pos);
		if (nGlobalPolyID >= 0)
		{
			int denseIdx = navGraph->GetDenseIndex(nGlobalPolyID);
			if (denseIdx >= 0) return denseIdx;
		}

		// 2. 스냅 후 재탐색
		Vector3 v3Snapped = navMesh->GetNearestPointOnNavMesh(pos);
		float fSnapDist = Vector3::Distance(pos, v3Snapped);

		nGlobalPolyID = navMesh->FindPolygonContaining(v3Snapped);
		if (nGlobalPolyID >= 0)
		{
			int nDenseIdx = navGraph->GetDenseIndex(nGlobalPolyID);
			if (nDenseIdx >= 0) return nDenseIdx;
		}

		return navGraph->FindClosestNode(pos);
	}


	// ─────────────────────────────────────────────────────────────────────────
	// RequestPathToPosition  : 비동기 A* 탐색 시작 및 PathManager 등록
	// ─────────────────────────────────────────────────────────────────────────
	bool AIPathPlanner::RequestPathToPosition(const Vector3& targetPos)
	{
		GetReadyForNewSearch();

		auto owner = m_wpOwner.lock();
		auto navGraph = m_wpNavGraph.lock();
		if (!owner || !navGraph)
			return false;

		m_v3Destination = targetPos;

		const Vector3 v3AgentPos = owner->GetPosition();

		m_nStartNode = GetNodeForPosition(v3AgentPos);
		m_nEndNode   = GetNodeForPosition(targetPos);

		if (m_nStartNode == no_node_found || m_nEndNode == no_node_found)
			return false;

		// 같은 폴리곤이거나 직선 시야 확보 → 즉시 직선 경로
		if (m_nStartNode == m_nEndNode)
		{
			std::list<PathEdge> DirectPath;
			DirectPath.emplace_back(v3AgentPos, targetPos);
			owner->OnPathReady(DirectPath);
			return true;
		}

		// A* 탐색 시작 (PathManager가 매 프레임 CycleOnce 호출)
		m_pCurrentSearch = std::make_unique<NavMeshAStar>(navGraph, m_nStartNode, m_nEndNode);

		if (auto pathManager = m_wpPathManager.lock())
			pathManager->Register(shared_from_this());

		return true;
	}


	// ─────────────────────────────────────────────────────────────────────────
	// CycleOnce  : PathManager 가 매 프레임 호출하는 탐색 진행 메서드
	// ─────────────────────────────────────────────────────────────────────────
	int AIPathPlanner::CycleOnce()
	{
		if (!m_pCurrentSearch)
			return target_not_found;

		auto owner    = m_wpOwner.lock();
		auto navGraph = m_wpNavGraph.lock();
		if (!owner || !navGraph)
			return target_not_found;

		int nResult = m_pCurrentSearch->CycleOnce();

		if (nResult == target_not_found)
		{
			owner->OnPathFailed();
		}
		else if (nResult == target_found)
		{
			std::list<int> PolySequence = m_pCurrentSearch->GetPathToTarget();

			// 디버그: A*가 찾은 폴리곤 중심점 저장
			{
				std::vector<Vector3> centers;
				centers.reserve(PolySequence.size());
				for (int denseIdx : PolySequence)
					centers.push_back(navGraph->GetNode(denseIdx).Pos());
				owner->m_DebugInfo.PolyNodeCenters = std::move(centers);
			}

			const Vector3 v3AgentPos = owner->GetPosition();
			std::list<PathEdge> Path = SmoothPath(PolySequence, v3AgentPos, m_v3Destination);
			owner->OnPathReady(Path);
		}

		return nResult;
	}


	// ─────────────────────────────────────────────────────────────────────────
	// SmoothPath  : Funnel Algorithm (String Pull)
	//
	//  A*가 찾은 폴리곤 코리더를 Funnel Algorithm으로 스무딩한다.
	//  포털(인접 폴리곤의 공유 엣지)을 순서대로 처리하며, 깔때기(funnel)로
	//  시야를 좁혀가다 코너가 꺾이면 그 꼭짓점을 경로점으로 추가한다.
	//
	//  포털의 Left/Right 분류는 GetSharedEdge 가 반환하는 CCW winding 순서에
	//  의해 고정적으로 결정된다 (EdgeA=Left, EdgeB=Right).
	// ─────────────────────────────────────────────────────────────────────────
	std::list<PathEdge> AIPathPlanner::SmoothPath(
		const std::list<int>& polySequence,
		const Vector3& startPos,
		const Vector3& endPos) const
	{
		auto navMesh = m_wpNavMesh.lock();
		auto navGraph = m_wpNavGraph.lock();
		auto owner = m_wpOwner.lock();

		auto fallback = [&]() -> std::list<PathEdge>
			{
				std::list<PathEdge> r;
				r.emplace_back(startPos, endPos);
				return r;
			};

		if (polySequence.empty() || !navMesh || !navGraph)
			return fallback();

		auto triArea2 = [](const Vector3& a, const Vector3& b, const Vector3& c) -> float
			{
				return (b.x - a.x) * (c.z - a.z)
					- (b.z - a.z) * (c.x - a.x);
			};

		std::vector<Portal> Portals;

		auto it1 = polySequence.begin();
		auto it2 = std::next(it1);
		while (it2 != polySequence.end())
		{
		    int nGidA = navGraph->GetGlobalPolyID(*it1);
		    int nGidB = navGraph->GetGlobalPolyID(*it2);

		    Vector3 v3Pos1, v3Pos2;
		    if (navMesh->GetSharedEdge(nGidA, nGidB, v3Pos1, v3Pos2))
		        Portals.push_back({ v3Pos1, v3Pos2 });

		    it1 = it2;
		    ++it2;
		}
		Portals.push_back({ endPos, endPos });

		// ── 2. Funnel Algorithm ───────────────────────────────────────────────
		std::vector<Vector3> Waypoints;
		Waypoints.push_back(startPos);

		Vector3 v3Apex = startPos;
		Vector3 v3LeftBound = startPos;
		Vector3 v3RightBound= startPos;
		int nLeftIdx  = 0;
		int nRightIdx = 0;

		int i = 0;
		while (i < (int)Portals.size())
		{
		    const Portal& rp = Portals[i];

			// GetSharedEdge 는 PolyA 의 CCW winding 순서로 outV1(EdgeA), outV2(EdgeB) 를 반환.
			// CCW 폴리곤에서 코리더를 따라 횡단할 때:
			//   EdgeA (v_i)     = Left  바운더리
			//   EdgeB (v_{i+1}) = Right 바운더리
			// 이 분류는 폴리곤 와인딩에 의해 결정되므로 고정이다.
			Vector3 v3NewLeft  = rp.v3EdgeA;
			Vector3 v3NewRight = rp.v3EdgeB;
		    if (i == (int)Portals.size() - 1)
		        v3NewLeft = v3NewRight = endPos;

		    // ── Right 업데이트 ─────────────────────────────────────────────
		    if (triArea2(v3Apex, v3RightBound, v3NewRight) <= g_fEpsilon)
		    {
		        if (v3RightBound == v3Apex || triArea2(v3Apex, v3LeftBound, v3NewRight) >= -g_fEpsilon)
		        {
		            v3RightBound = v3NewRight;
		            nRightIdx = i;
		        }
		        else
		        {
		            Waypoints.push_back(v3LeftBound);
		            v3Apex = v3LeftBound;
		            int nNewApexIdx = nLeftIdx;
		            v3LeftBound = v3Apex;
		            v3RightBound = v3Apex;
		            nLeftIdx = nNewApexIdx;
		            nRightIdx = nNewApexIdx;
		            i = nNewApexIdx + 1;
		            continue;
		        }
		    }

		    // ── Left 업데이트 ──────────────────────────────────────────────
		    if (triArea2(v3Apex, v3LeftBound, v3NewLeft) >= -g_fEpsilon)
		    {
		        if (v3LeftBound == v3Apex || triArea2(v3Apex, v3RightBound, v3NewLeft) <= g_fEpsilon)
		        {
		            v3LeftBound = v3NewLeft;
		            nLeftIdx = i;
		        }
		        else
		        {
		            Waypoints.push_back(v3RightBound);
		            v3Apex = v3RightBound;
		            int nNewApexIdx = nRightIdx;
		            v3LeftBound = v3Apex;
		            v3RightBound = v3Apex;
		            nLeftIdx = nNewApexIdx;
		            nRightIdx = nNewApexIdx;
		            i = nNewApexIdx + 1;
		            continue;
		        }
		    }

		    ++i;
		}

		Waypoints.push_back(endPos);

		// ── 3. 디버그 정보 저장 ───────────────────────────────────────────
		if (owner)
		{
			owner->m_DebugInfo.Waypoints.assign(Waypoints.begin(), Waypoints.end());
			owner->m_DebugInfo.v3StartPos = startPos;
			owner->m_DebugInfo.v3EndPos = endPos;

			owner->m_DebugInfo.Portals.clear();
			auto pit1 = polySequence.begin();
			auto pit2 = std::next(pit1);
			for (int k = 0; k + 1 < (int)Portals.size(); ++k)
			{
				PathDebugInfo::Portal pdi;
				pdi.v3Left = Portals[k].v3EdgeA;
				pdi.v3Right = Portals[k].v3EdgeB;
				pdi.nPolyA = *pit1;
				pdi.nPolyB = *pit2;
				owner->m_DebugInfo.Portals.push_back(pdi);
				++pit1;
				++pit2;
			}
		}

		// ── 4. PathEdge 변환 ──────────────────────────────────────────────
		std::list<PathEdge> ResultPath;
		for (int k = 0; k + 1 < (int)Waypoints.size(); ++k)
			ResultPath.emplace_back(Waypoints[k], Waypoints[k + 1]);

		if (ResultPath.empty())
			ResultPath.emplace_back(startPos, endPos);

		return ResultPath;
	}

} // namespace AIDLL
