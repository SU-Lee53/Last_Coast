#pragma once
//------------------------------------------------------------------------
//
//  Name:   TimeSlicedGraphAlgorithms.h
//
//  Desc:   NavMesh A* 탐색기 (시간 분할 방식)
//          매 프레임 CycleOnce() 를 호출해 탐색을 조금씩 진행한다.
//          PathManager 에 의해 관리된다.
//
//------------------------------------------------------------------------
#include "pch.h"
#include "IndexedPriorityQueue.h"
#include "NavMeshGraph.h"
#include "PathEdge.h"





//-------------------------- NavMeshAStar -------------------------------------
//
//  NavMeshGraph 위에서 동작하는 A* 탐색기.
//  여러 프레임에 걸쳐 분산 실행된다.
//
//  CycleOnce()      : 탐색 한 스텝 진행. target_found / target_not_found /
//                     search_incomplete 중 하나를 반환한다.
//  GetPathToTarget(): 탐색 완료 후 시작→목적지 Dense 노드 인덱스 리스트 반환.
//-----------------------------------------------------------------------------
namespace AIDLL
{
	enum State
	{
		target_found,
		target_not_found,
		search_incomplete
	};

	class NavMeshAStar
	{
	private:

		std::shared_ptr<const NavMeshGraph> m_Graph;

		std::vector<double>              m_GCosts;
		std::vector<double>              m_FCosts;
		std::vector<const NavMeshEdge*>  m_ShortestPathTree;
		std::vector<const NavMeshEdge*>  m_SearchFrontier;

		int m_nSource;
		int m_nTarget;

		// raw pointer → unique_ptr 로 교체
		// (IndexedPriorityQLow 는 m_FCosts 의 참조를 들고 있으므로
		//  m_FCosts 보다 먼저 선언되면 안 됨 → 멤버 선언 순서 유지)
		std::unique_ptr<IndexedPriorityQLow<double>> m_pPQ;

	public:

		NavMeshAStar(std::shared_ptr<const NavMeshGraph> graph, int source, int target)
			: m_Graph(graph)
			, m_GCosts(graph->NumNodes(), 0.0)
			, m_FCosts(graph->NumNodes(), 0.0)
			, m_ShortestPathTree(graph->NumNodes(), nullptr)
			, m_SearchFrontier(graph->NumNodes(), nullptr)
			, m_nSource(source)
			, m_nTarget(target)
			, m_pPQ(std::make_unique<IndexedPriorityQLow<double>>(m_FCosts, graph->NumNodes()))
		{
			m_pPQ->insert(m_nSource);
		}

		// unique_ptr 덕분에 소멸자 직접 구현 불필요
		~NavMeshAStar() = default;

		// 복사/이동 금지 (우선순위 큐가 m_FCosts 참조를 들고 있음)
		NavMeshAStar(const NavMeshAStar&) = delete;
		NavMeshAStar& operator=(const NavMeshAStar&) = delete;

		int CycleOnce();
		std::list<int>  GetPathToTarget() const;
		std::list<PathEdge> GetPathAsPathEdges() const;
		double GetCostToTarget() const { return m_GCosts[m_nTarget]; }
	};


	//-----------------------------------------------------------------------------
	inline int NavMeshAStar::CycleOnce()
	{
		if (m_pPQ->empty())
			return target_not_found;

		int nNext = m_pPQ->Pop();
		m_ShortestPathTree[nNext] = m_SearchFrontier[nNext];

		if (nNext == m_nTarget)
			return target_found;

		NavMeshGraph::ConstEdgeIterator EdgeItr(m_Graph , nNext);
		for (auto pE = EdgeItr.begin(); !EdgeItr.end(); pE = EdgeItr.next())
		{
			double dHCost = static_cast<double>(
				Vector3::Distance(m_Graph->GetNode(m_nTarget).Pos(),
					m_Graph->GetNode(pE->To()).Pos()));
			double dGCost = m_GCosts[nNext] + pE->Cost();

			if (m_SearchFrontier[pE->To()] == nullptr)
			{
				m_FCosts[pE->To()] = dGCost + dHCost;
				m_GCosts[pE->To()] = dGCost;
				m_pPQ->insert(pE->To());
				m_SearchFrontier[pE->To()] = pE;
			}
			else if (dGCost < m_GCosts[pE->To()] && m_ShortestPathTree[pE->To()] == nullptr)
			{
				m_FCosts[pE->To()] = dGCost + dHCost;
				m_GCosts[pE->To()] = dGCost;
				m_pPQ->ChangePriority(pE->To());
				m_SearchFrontier[pE->To()] = pE;
			}
		}

		return search_incomplete;
	}


	//-----------------------------------------------------------------------------
	inline std::list<int> NavMeshAStar::GetPathToTarget() const
	{
		std::list<int> Path;
		if (m_nTarget < 0) return Path;

		int nNode = m_nTarget;
		Path.push_back(nNode);

		while (nNode != m_nSource && m_ShortestPathTree[nNode] != nullptr)
		{
			nNode = m_ShortestPathTree[nNode]->From();
			Path.push_front(nNode);
		}

		return Path;
	}


	//-----------------------------------------------------------------------------
	inline std::list<PathEdge> NavMeshAStar::GetPathAsPathEdges() const
	{
		std::list<PathEdge> Path;
		if (m_nTarget < 0) 
			return Path;

		int nd = m_nTarget;

		while (nd != m_nSource && m_ShortestPathTree[nd] != nullptr)
		{
			const auto e = m_ShortestPathTree[nd];
			Path.push_front(PathEdge(
				m_Graph->GetNode(e->From()).Pos(),
				m_Graph->GetNode(e->To()).Pos()
			));
			nd = e->From();
		}

		return Path;
	}

} // namespace AIDLL
