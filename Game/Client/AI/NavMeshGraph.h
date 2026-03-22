#pragma once
//-----------------------------------------------------------------------------
//
//  Name:   NavMeshGraph.h
//
//  Desc:   NavMeshImpl 의 폴리곤 인접 그래프를 TimeSlicedGraphAlgorithms 에서
//          사용할 수 있도록 Dense-indexed 형태로 래핑하는 어댑터.
//
//          NavMeshImpl::BuildGraph() 가 이 타입의 객체를 생성해 반환한다.
//
//-----------------------------------------------------------------------------
#include "pch.h"

namespace AIDLL
{

	// ─────────────────────────────────────────────────────────────────────────────
	// NavMeshEdge  – 폴리곤 간 이동 엣지 (Graph_SearchAStar_TS 인터페이스 준수)
	// ─────────────────────────────────────────────────────────────────────────────
	struct NavMeshEdge
	{
		int nFrom;
		int nTo;
		float fCost;

		int From() const { return nFrom; }
		int To() const { return nTo; }
		float Cost() const { return fCost; }
	};


	// ─────────────────────────────────────────────────────────────────────────────
	// NavMeshNode  – 폴리곤 무게중심을 위치로 갖는 그래프 노드
	// ─────────────────────────────────────────────────────────────────────────────
	struct NavMeshNode
	{
		int nIndex;
		Vector3 v3Positon;       // 폴리곤 무게중심 (3D)

		int Index() const { return nIndex; }
		Vector3 Pos() const { return v3Positon; }
	};


	// ─────────────────────────────────────────────────────────────────────────────
	// NavMeshGraph  – TimeSlicedGraphAlgorithms 호환 그래프 어댑터
	//
	//  Dense 인덱스 체계 사용:
	//    index 0 = 타일0의 폴리곤0
	//    index 1 = 타일0의 폴리곤1  ...
	//    index M = 타일1의 폴리곤0  ...
	//
	//  NavMeshImpl::BuildGraph() 에서 생성된다.
	// ─────────────────────────────────────────────────────────────────────────────
	class NavMeshGraph
	{
	public:

		// 노드 (폴리곤 무게중심) 목록 – Dense index 순서
		std::vector<NavMeshNode> m_Nodes;

		// 인접 엣지 목록 – m_Adjacency[denseIdx] = 이웃 엣지 목록
		std::vector<std::vector<NavMeshEdge>> m_Adjacency;

		// Sparse global polyID → Dense index 변환 테이블
		std::unordered_map<int, int> m_SparseToDense;

		// Dense index → Sparse global polyID 역방향 테이블 (String Pulling 용)
		std::vector<int> m_DenseToSparse;


		// ── 기본 인터페이스 ─────────────────────────────────────────────────────

		int NumNodes() const { return static_cast<int>(m_Nodes.size()); }

		const NavMeshNode& GetNode(int idx) const { return m_Nodes[idx]; }

		// Sparse global polyID → Dense index (없으면 -1)
		int GetDenseIndex(int globalPolyID) const
		{
			auto it = m_SparseToDense.find(globalPolyID);
			return (it != m_SparseToDense.end()) ? it->second : -1;
		}

		// Dense index → Sparse global polyID (없으면 -1)
		int GetGlobalPolyID(int denseIdx) const
		{
			if (denseIdx < 0 || denseIdx >= static_cast<int>(m_DenseToSparse.size()))
				return -1;
			return m_DenseToSparse[denseIdx];
		}

		// 위치에서 가장 가까운 폴리곤 노드의 Dense index 반환
		int FindClosestNode(const Vector3& pos) const
		{
			int nBest = -1;
			float fBestDist = FLT_MAX;

			for (int i = 0; i < static_cast<int>(m_Nodes.size()); ++i)
			{
				float d = Vector3::Distance(pos, m_Nodes[i].v3Positon);
				if (d < fBestDist)
				{
					fBestDist = d;
					nBest = i;
				}
			}

			return nBest;
		}


		// ── ConstEdgeIterator ───────────────────────────────────────────────────
		// Graph_SearchAStar_TS 가 요구하는 엣지 순회 인터페이스
		class ConstEdgeIterator
		{
		private:

			std::shared_ptr<const NavMeshGraph> m_Graph;
			int m_nNodeIdx;
			int m_nEdgeIdx;

			const NavMeshEdge* current() const
			{
				if (end())
					return nullptr;
				return &m_Graph->m_Adjacency[m_nNodeIdx][m_nEdgeIdx];
			}

		public:

			ConstEdgeIterator(std::shared_ptr<const NavMeshGraph> G, int nodeIdx) : m_Graph(G), m_nNodeIdx(nodeIdx), m_nEdgeIdx(0)
			{

			}

			const NavMeshEdge* begin()
			{
				m_nEdgeIdx = 0;
				return current();
			}

			bool end() const
			{
				return m_nNodeIdx < 0 ||
					m_nNodeIdx >= static_cast<int>(m_Graph->m_Adjacency.size()) ||
					m_nEdgeIdx >= static_cast<int>(m_Graph->m_Adjacency[m_nNodeIdx].size());
			}

			const NavMeshEdge* next()
			{
				++m_nEdgeIdx;
				return current();
			}
		};
	};
} // namespace AI
