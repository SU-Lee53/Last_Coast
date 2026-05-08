#include "pch.h"
#include <random>
#include "NavMeshImpl.h"


// ─────────────────────────────────────────────────────────────────────────────
// 내부 구현용 헬퍼 (익명 네임스페이스)
// ─────────────────────────────────────────────────────────────────────────────


namespace AIDLL
{
	NavMeshImpl::NavMeshImpl()
	{
	}

	NavMeshImpl::~NavMeshImpl()
	{
	}

	// ─────────────────────────────────────────────────────────────────────────
	// LoadFromJson
	// ─────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::LoadFromJson(const std::string& strFileName)
	{
		// 확장자가 이미 포함된 경로면 그대로 사용, 아니면 BasePath + 이름 + .json 조합
		std::string strFilePath;
		if (strFileName.size() >= 5 &&
			strFileName.substr(strFileName.size() - 5) == ".json")
			strFilePath = strFileName;
		else
			strFilePath = std::format("{}/{}.json", g_strAIBasePath, strFileName);

		std::ifstream inFile{ strFilePath, std::ios::binary };
		if (!inFile) {
			return false;
		}

		nlohmann::json jNavMesh = nlohmann::json::parse(inFile);

		// Config 로드
		if (jNavMesh.contains("Config"))
		{
			auto& jConfig = jNavMesh["Config"];
			m_Config.fCellSize = jConfig["CellSize"].get<float>();
			m_Config.fCellHeight = jConfig["CellHeight"].get<float>();
			m_Config.fAgentRadius = jConfig["AgentRadius"].get<float>();
			m_Config.fAgentHeight = jConfig["AgentHeight"].get<float>();
			m_Config.fAgentMaxSlope = jConfig["AgentMaxSlope"].get<float>();
			m_Config.fAgentMaxStepHeight = jConfig["AgentMaxStepHeight"].get<float>();
		}

		// Tiles 로드
		m_Tiles.clear();
		if (jNavMesh.contains("Tiles"))
		{
			for (auto& jTile : jNavMesh["Tiles"])
			{
				NavMeshTile Tile;
				Tile.nX = jTile["X"].get<int>();
				Tile.nY = jTile["Y"].get<int>();
				Tile.nLayer = jTile["Layer"].get<int>();

				auto& jBounds = jTile["Bounds"];
				Tile.v3BoundsMin = Vector3(
					jBounds["Min"]["X"].get<float>(),
					jBounds["Min"]["Y"].get<float>(),
					jBounds["Min"]["Z"].get<float>()
				);
				Tile.v3BoundsMax = Vector3(
					jBounds["Max"]["X"].get<float>(),
					jBounds["Max"]["Y"].get<float>(),
					jBounds["Max"]["Z"].get<float>()
				);

				for (auto& jVert : jTile["Vertices"])
				{
					NavMeshVertex Vertex;
					Vertex.v3Position = Vector3(
						jVert["X"].get<float>(),
						jVert["Y"].get<float>(),
						jVert["Z"].get<float>()
					);
					Tile.Vertices.push_back(Vertex);
				}

				for (auto& jPoly : jTile["Polygons"])
				{
					NavMeshPolygon Polygon;
					for (auto& jIndex : jPoly["Indices"]) {
						Polygon.Indices.push_back(jIndex.get<int>());
					}
					Polygon.nAreaType = jPoly["AreaType"].get<int>();
					Polygon.nFlags    = jPoly["Flags"].get<int>();
					Tile.Polygons.push_back(Polygon);
				}

				m_Tiles.push_back(Tile);
			}
		}

		// 로드 완료 후 인접 그래프 구축
		BuildAdjacency();

		return true;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// BuildAdjacency
	// 엣지(두 정점의 위치)를 공유하는 폴리곤끼리 인접으로 등록
	// 타일 경계를 넘어서도 정점 위치가 같으면 인접으로 처리
	// ─────────────────────────────────────────────────────────────────────────
	void NavMeshImpl::BuildAdjacency()
	{
		m_Adjacency.clear();

		// 엣지 -> 처음 이 엣지를 등록한 전역 PolyID
		std::unordered_map<EdgeKey, int, EdgeKeyHash> EdgeToPolyID;

		for (int tileIdx = 0; tileIdx < (int)m_Tiles.size(); ++tileIdx)
		{
			const auto& Tile = m_Tiles[tileIdx];
			for (int polyIdx = 0; polyIdx < (int)Tile.Polygons.size(); ++polyIdx)
			{
				const auto& Poly = Tile.Polygons[polyIdx];
				int nGlobalID = MakePolyID(tileIdx, polyIdx);

				// 엔트리 확보
				m_Adjacency.emplace(nGlobalID, std::vector<int>{});

				int nSize = (int)Poly.Indices.size();
				for (int i = 0; i < nSize; i++)
				{
					int idx1 = Poly.Indices[i];
					int idx2 = Poly.Indices[(i + 1) % nSize];

					if (idx1 >= (int)Tile.Vertices.size() || idx2 >= (int)Tile.Vertices.size()) {
						continue;
					}

					const Vector3& v3Pos1 = Tile.Vertices[idx1].v3Position;
					const Vector3& v3Pos2 = Tile.Vertices[idx2].v3Position;

					EdgeKey Edge;
					Edge.nX1 = static_cast<int64>(v3Pos1.x * QUANT_SCALE);
					Edge.nY1 = static_cast<int64>(v3Pos1.y * QUANT_SCALE);
					Edge.nZ1 = static_cast<int64>(v3Pos1.z * QUANT_SCALE);
					Edge.nX2 = static_cast<int64>(v3Pos2.x * QUANT_SCALE);
					Edge.nY2 = static_cast<int64>(v3Pos2.y * QUANT_SCALE);
					Edge.nZ2 = static_cast<int64>(v3Pos2.z * QUANT_SCALE);

					auto it = EdgeToPolyID.find(Edge);
					if (it == EdgeToPolyID.end())
					{
						// 처음 보는 엣지: 나중에 이웃을 찾기 위해 등록
						EdgeToPolyID[Edge] = nGlobalID;
					}
					else
					{
						// 이미 다른 폴리곤이 이 엣지를 갖고 있음 → 서로 인접
						int nNeighborID = it->second;
						if (nNeighborID != nGlobalID)
						{
							m_Adjacency[nGlobalID].push_back(nNeighborID);
							m_Adjacency[nNeighborID].push_back(nGlobalID);
						}
						// 3개 이상의 폴리곤이 같은 엣지를 공유하는 경우는 없으므로 삭제
						EdgeToPolyID.erase(it);
					}
				}
			}
		}
	}

	// ─────────────────────────────────────────────────────────────────────────
	// GetPolygonCenter
	// ─────────────────────────────────────────────────────────────────────────
	Vector3 NavMeshImpl::GetPolygonCenter(int tileIdx, int polyIdx) const
	{
		if (tileIdx < 0 || tileIdx >= (int)m_Tiles.size()) {
			return Vector3::Zero;
		}

		const auto& Tile = m_Tiles[tileIdx];
		if (polyIdx < 0 || polyIdx >= (int)Tile.Polygons.size()) {
			return Vector3::Zero;
		}

		const auto& Poly = Tile.Polygons[polyIdx];
		Vector3 v3Sum = Vector3::Zero;
		int nCount = 0;

		for (int idx : Poly.Indices)
		{
			if (idx >= 0 && idx < (int)Tile.Vertices.size())
			{
				v3Sum += Tile.Vertices[idx].v3Position;
				nCount++;
			}
		}

		return (nCount > 0) ? (v3Sum / static_cast<float>(nCount)) : Vector3::Zero;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// FindPolygonContaining
	// point가 속하는 폴리곤의 전역 PolyID 반환, 없으면 -1
	// ─────────────────────────────────────────────────────────────────────────
	int NavMeshImpl::FindPolygonContaining(const Vector3& point) const
	{
		for (int tileIdx = 0; tileIdx < m_Tiles.size(); ++tileIdx)
		{
			const auto& Tile = m_Tiles[tileIdx];

			// AABB 빠른 기각 (XZ 평면 기준)
			if (point.x < Tile.v3BoundsMin.x || point.x > Tile.v3BoundsMax.x ||
				point.z < Tile.v3BoundsMin.z || point.z > Tile.v3BoundsMax.z)
				continue;

			for (int polyIdx = 0; polyIdx < (int)Tile.Polygons.size(); polyIdx++)
			{
				if (IsPointInPolygon(point, Tile.Polygons[polyIdx], Tile))
					return MakePolyID(tileIdx, polyIdx);
			}
		}
		return -1;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// IsPointOnNavMesh
	// ─────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::IsPointOnNavMesh(const Vector3& point) const
	{
		return FindPolygonContaining(point) >= 0;
	}


	Vector3 NavMeshImpl::ClosestPointOnSegment(const Vector3& p, const Vector3& a, const Vector3& b)
	{
		Vector3 v3Edge = b - a;
		float fLenSq = v3Edge.Dot(v3Edge);

		if (fLenSq < 1e-6f)
			return a;

		float fSegT = v3Edge.Dot(p - a) / fLenSq;
		fSegT = std::max(0.0f, std::min(1.0f, fSegT));
		return a + v3Edge * fSegT;
	}

	// 폴리곤까지의 최단 거리와 그 점을 계산
	std::pair<float, Vector3> NavMeshImpl::ClosestPointOnPolygon(const Vector3& p,
		const NavMeshPolygon& poly,
		const NavMeshTile& tile) const
	{
		if (poly.Indices.size() < 3)
			return { FLT_MAX, p };

		float fAvgY = 0.0f;
		for (int idx : poly.Indices)
		{
			if (idx >= 0 && idx < (int)tile.Vertices.size())
				fAvgY += tile.Vertices[idx].v3Position.y;
		}
		fAvgY /= poly.Indices.size();

		if (IsPointInPolygon(p, poly, tile))
		{
			Vector3 v3Projected = p;
			v3Projected.y = fAvgY;
			return { std::abs(p.y - fAvgY), v3Projected };
		}

		float fMinDist = FLT_MAX;
		Vector3 v3Closest = p;

		for (size_t i = 0; i < poly.Indices.size(); ++i)
		{
			int nIdx1 = poly.Indices[i];
			int nIdx2 = poly.Indices[(i + 1) % poly.Indices.size()];

			if (nIdx1 < 0 || nIdx1 >= (int)tile.Vertices.size() ||
				nIdx2 < 0 || nIdx2 >= (int)tile.Vertices.size())
				continue;

			const Vector3& v3Pos1 = tile.Vertices[nIdx1].v3Position;
			const Vector3& v3Pos2 = tile.Vertices[nIdx2].v3Position;

			Vector3 v3PointOnEdge = ClosestPointOnSegment(p, v3Pos1, v3Pos2);
			float fDist = Vector3::Distance(p, v3PointOnEdge);

			if (fDist < fMinDist)
			{
				fMinDist = fDist;
				v3Closest = v3PointOnEdge;
			}
		}

		return { fMinDist, v3Closest };
	}
	// ─────────────────────────────────────────────────────────────────────────
	// GetNearestPointOnNavMesh
	// XZ 평면에서 가장 가까운 폴리곤 표면 위의 점을 반환 (Y는 폴리곤 높이 사용)
	// ─────────────────────────────────────────────────────────────────────────
	// 선분 위의 가장 가까운 점을 계산
	Vector3 NavMeshImpl::GetNearestPointOnNavMesh(const Vector3& point) const
	{
		float fMinDistance = FLT_MAX;
		Vector3 v3NearestPoint = point;

		for (const auto& tile : m_Tiles)
		{
			for (const auto& poly : tile.Polygons)
			{
				auto [dist, closest] = ClosestPointOnPolygon(point, poly, tile);
				if (dist < fMinDistance)
				{
					fMinDistance = dist;
					v3NearestPoint = closest;
				}
			}
		}

		return v3NearestPoint;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// IsPointInPolygon  (XZ 평면 레이캐스팅 방식)
	// ─────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::IsPointInPolygon(const Vector3& point,const NavMeshPolygon& poly, const NavMeshTile& tile) const
	{
		if (poly.Indices.size() < 3)
			return false;

		int nCrossings = 0;
		int nSize = (int)poly.Indices.size();

		for (int i = 0; i < nSize; i++)
		{
			int nIdx1 = poly.Indices[i];
			int nIdx2 = poly.Indices[(i + 1) % nSize];

			if (nIdx1 >= (int)tile.Vertices.size() ||
				nIdx2 >= (int)tile.Vertices.size())
				continue;

			const Vector3& v3Pos1 = tile.Vertices[nIdx1].v3Position;
			const Vector3& v3Pos2 = tile.Vertices[nIdx2].v3Position;

			if (((v3Pos1.z <= point.z && point.z < v3Pos2.z) ||
				(v3Pos2.z <= point.z && point.z < v3Pos1.z)) &&
				(point.x < (v3Pos2.x - v3Pos1.x) * (point.z - v3Pos1.z) / (v3Pos2.z - v3Pos1.z) + v3Pos1.x))
			{
				nCrossings++;
			}
		}

		return (nCrossings % 2) == 1;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// BuildGraph  (NavMeshGraph 생성)
	// 폴리곤마다 Dense index 를 부여하고 인접 엣지를 구성한다.
	// AIManagerImpl::LoadNavMesh() 에서 호출된다.
	// ─────────────────────────────────────────────────────────────────────────
	std::unique_ptr<NavMeshGraph> NavMeshImpl::BuildGraph() const
	{
		auto Graph = std::make_unique<NavMeshGraph>();

		// ── 1단계: Dense index 부여 및 노드 생성 ──────────────────────────────
		int nDenseIdx = 0;
		for (int tileIdx = 0; tileIdx < static_cast<int>(m_Tiles.size()); ++tileIdx)
		{
			for (int polyIdx = 0; polyIdx < static_cast<int>(m_Tiles[tileIdx].Polygons.size()); ++polyIdx)
			{
				int nGlobalID = MakePolyID(tileIdx, polyIdx);
				Graph->m_SparseToDense[nGlobalID] = nDenseIdx;
				Graph->m_DenseToSparse.push_back(nGlobalID);

				NavMeshNode Node;
				Node.nIndex = nDenseIdx;
				Node.v3Positon   = GetPolygonCenter(tileIdx, polyIdx);
				Graph->m_Nodes.push_back(Node);

				++nDenseIdx;
			}
		}

		// ── 2단계: 인접 엣지 구성 ────────────────────────────────────────────
		Graph->m_Adjacency.resize(nDenseIdx);

		for (const auto& [globalID, neighbors] : m_Adjacency)
		{
			auto FromIt = Graph->m_SparseToDense.find(globalID);
			if (FromIt == Graph->m_SparseToDense.end()) continue;

			int nFromDense = FromIt->second;
			const Vector3& v3FromPos = Graph->m_Nodes[nFromDense].v3Positon;

			for (int neighborGlobalID : neighbors)
			{
				auto ToIt = Graph->m_SparseToDense.find(neighborGlobalID);
				if (ToIt == Graph->m_SparseToDense.end()) continue;

				int nToDense = ToIt->second;
				const Vector3& ToPos = Graph->m_Nodes[nToDense].v3Positon;

				NavMeshEdge Edge;
				Edge.nFrom = nFromDense;
				Edge.nTo = nToDense;
				Edge.fCost = static_cast<double>(Vector3::Distance(v3FromPos, ToPos));

				Graph->m_Adjacency[nFromDense].push_back(Edge);
			}
		}

		return Graph;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// GetSharedEdge
	// 두 인접 폴리곤이 공유하는 엣지의 두 정점을 반환한다.
	// String Pulling(Funnel 알고리즘)에서 포털 좌우 정점을 구할 때 사용.
	//
	// 반환 순서: 폴리곤 A의 winding order에 따라 결정
	// (폴리곤이 CCW 순서로 정의되어 있다면, A→B 진행 시 자연스러운 순서)
	// ─────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::GetSharedEdge(int globalPolyID_A, int globalPolyID_B,
	                                Vector3& outV1, Vector3& outV2) const
	{
		int nTileA = GetTileFromID(globalPolyID_A);
		int nPolyA = GetPolyFromID(globalPolyID_A);
		int nTileB = GetTileFromID(globalPolyID_B);
		int nPolyB = GetPolyFromID(globalPolyID_B);

		if (nTileA < 0 || nTileA >= (int)m_Tiles.size()) return false;
		if (nTileB < 0 || nTileB >= (int)m_Tiles.size()) return false;

		const auto& TileA = m_Tiles[nTileA];
		const auto& TileB = m_Tiles[nTileB];

		if (nPolyA < 0 || nPolyA >= (int)TileA.Polygons.size()) return false;
		if (nPolyB < 0 || nPolyB >= (int)TileB.Polygons.size()) return false;

		const auto& PolyA = TileA.Polygons[nPolyA];
		const auto& PolyB = TileB.Polygons[nPolyB];

		std::vector<SharedVertex> SharedVerts;
		SharedVerts.reserve(2);

		// 폴리곤 A의 정점을 순서대로 검사
		for (int idxA = 0; idxA < (int)PolyA.Indices.size(); ++idxA)
		{
			int nIndexA = PolyA.Indices[idxA];
			if (nIndexA < 0 || nIndexA >= (int)TileA.Vertices.size()) continue;
			const Vector3& v3PosA = TileA.Vertices[nIndexA].v3Position;

			// 폴리곤 B에서 같은 위치의 정점 찾기
			for (int iB : PolyB.Indices)
			{
				if (iB < 0 || iB >= (int)TileB.Vertices.size()) continue;
				const Vector3& v3PosB = TileB.Vertices[iB].v3Position;

				// BuildAdjacency 와 동일한 1mm 정밀도 비교
				int64 nDeltaX = static_cast<int64>((v3PosA.x - v3PosB.x) * QUANT_SCALE);
				int64 nDeltaY = static_cast<int64>((v3PosA.y - v3PosB.y) * QUANT_SCALE);
				int64 nDeltaZ = static_cast<int64>((v3PosA.z - v3PosB.z) * QUANT_SCALE);

				if (nDeltaX == 0 && nDeltaY == 0 && nDeltaZ == 0)
				{
					SharedVerts.push_back({ v3PosA, idxA });
					break;
				}
			}

			if (SharedVerts.size() >= 2)
				break;
		}

		if (SharedVerts.size() < 2)
			return false;

		// 폴리곤 A의 winding order에 따라 정점 순서 결정
		// 연속된 엣지인 경우 (idx 차이가 1) 순서대로, 아니면 wrap-around 처리
		int nIdx0 = SharedVerts[0].nIndexInPolyA;
		int nIdx1 = SharedVerts[1].nIndexInPolyA;
		int nSize = (int)PolyA.Indices.size();

		// idx0 다음이 idx1인지 확인 (CCW winding order 가정)
		bool bIsSequential = (nIdx1 == (nIdx0 + 1) % nSize);

		if (bIsSequential)
		{
			// 순서대로
			outV1 = SharedVerts[0].v3Pos;
			outV2 = SharedVerts[1].v3Pos;
		}
		else
		{
			// 역순 (idx1 다음이 idx0인 경우)
			outV1 = SharedVerts[1].v3Pos;
			outV2 = SharedVerts[0].v3Pos;
		}

		return true;
	}

	// ─────────────────────────────────────────────────────────────────────────
	// DistanceToPolygon  (가장 가까운 정점까지의 거리)
	// ─────────────────────────────────────────────────────────────────────────
	float NavMeshImpl::DistanceToPolygon(const Vector3& point,const NavMeshPolygon& poly, const NavMeshTile& tile) const
	{
		float fMinDist = FLT_MAX;

		for (int idx : poly.Indices)
		{
			if (idx >= 0 && idx < (int)tile.Vertices.size())
			{
				float fDist = Vector3::Distance(point, tile.Vertices[idx].v3Position);
				fMinDist = std::min(fMinDist, fDist);
			}
		}

		return fMinDist;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// SegmentIntersect2D  : XZ 평면 선분 교차 판정 (IsLineOfSightClear 전용 헬퍼)
	// ray  = P1 + t*(P2-P1),  edge = P3 + s*(P4-P3)
	// 반환값: 두 선분이 평행하지 않으면 true (t, s 파라미터 설정)
	// ─────────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::SegmentIntersect2D(const Vector3& P1, const Vector3& P2,
	                                     const Vector3& P3, const Vector3& P4,
	                                     float& OutT, float& OutS)
	{
		const float fD1x = P2.x - P1.x,  fD1z = P2.z - P1.z;
		const float fD2x = P4.x - P3.x,  fD2z = P4.z - P3.z;
		const float fDenom = fD1x * fD2z - fD1z * fD2x;

		if (std::abs(fDenom) < 1e-8f)
			return false;  // 평행

		const float fDx = P3.x - P1.x,  fDz = P3.z - P1.z;
		OutT = (fDx * fD2z - fDz * fD2x) / fDenom;
		OutS = (fDx * fD1z - fDz * fD1x) / fDenom;
		return true;
	}


	// ─────────────────────────────────────────────────────────────────────────────
	// IsLineOfSightClear  : From → To 직선이 NavMesh 경계(벽)에 가로막히지
	//                       않으면 true 반환 (XZ 평면 폴리곤 순회 방식)
	//
	// 알고리즘:
	//   1. From 이 속한 폴리곤에서 출발
	//   2. 각 이터레이션에서 To 가 현재 폴리곤 안에 있으면 → true (시야 확보)
	//   3. ray 가 가장 먼저 교차하는 폴리곤 엣지를 탐색 (최소 t)
	//   4. 그 엣지가 공유 엣지(인접 폴리곤 존재) → 이웃 폴리곤으로 이동, 반복
	//   5. 그 엣지가 경계 엣지(이웃 없음) → 벽 → false 반환
	// ─────────────────────────────────────────────────────────────────────────────
	bool NavMeshImpl::IsLineOfSightClear(const Vector3& From, const Vector3& To) const
	{
		int nCurrentPolyID = FindPolygonContaining(From);
		if (nCurrentPolyID < 0)
			return false;  // 시작점이 NavMesh 위에 없음

		constexpr int   nMaxIter  = 512;
		constexpr float fEps      = 1e-3f;
		float fCurrentT = 0.0f;  // ray 위 현재 진행 파라미터

		for (int nIter = 0; nIter < nMaxIter; ++nIter)
		{
			const int nTileIdx = GetTileFromID(nCurrentPolyID);
			const int nPolyIdx = GetPolyFromID(nCurrentPolyID);

			if (nTileIdx < 0 || nTileIdx >= (int)m_Tiles.size()) return false;
			const NavMeshTile& Tile = m_Tiles[nTileIdx];
			if (nPolyIdx < 0 || nPolyIdx >= (int)Tile.Polygons.size()) return false;
			const NavMeshPolygon& Poly = Tile.Polygons[nPolyIdx];

			// 목표가 현재 폴리곤 안이면 시야 확보
			if (IsPointInPolygon(To, Poly, Tile))
				return true;

			// 현재 폴리곤 모든 엣지 중 ray 가 가장 먼저 교차하는 엣지 탐색
			int   nMinEdgeIdx = -1;
			float fMinT       = FLT_MAX;
			const int nNumVerts = (int)Poly.Indices.size();

			for (int nE = 0; nE < nNumVerts; ++nE)
			{
				const int nIdx1 = Poly.Indices[nE];
				const int nIdx2 = Poly.Indices[(nE + 1) % nNumVerts];
				if (nIdx1 < 0 || nIdx1 >= (int)Tile.Vertices.size()) continue;
				if (nIdx2 < 0 || nIdx2 >= (int)Tile.Vertices.size()) continue;

				float fT, fS;
				if (!SegmentIntersect2D(From, To,
				                        Tile.Vertices[nIdx1].v3Position,
				                        Tile.Vertices[nIdx2].v3Position,
				                        fT, fS))
					continue;

				if (fT <= fCurrentT + fEps)    continue;  // 이미 지난 엣지
				if (fT  > 1.0f      + fEps)    continue;  // 목표 너머
				if (fS  < -fEps || fS > 1.0f + fEps) continue;  // 엣지 범위 밖

				if (fT < fMinT)
				{
					fMinT       = fT;
					nMinEdgeIdx = nE;
				}
			}

			// t ≤ 1.0 인 교차 엣지가 없으면 → ray 가 이 폴리곤 안에서 종료 (통과)
			if (nMinEdgeIdx < 0)
				return true;

			// 해당 엣지가 공유 엣지인지 확인 (인접 폴리곤 탐색)
			const Vector3& v3ExitE1 = Tile.Vertices[Poly.Indices[nMinEdgeIdx]].v3Position;
			const Vector3& v3ExitE2 = Tile.Vertices[Poly.Indices[(nMinEdgeIdx + 1) % nNumVerts]].v3Position;

			int nNextPolyID = -1;
			auto adjIt = m_Adjacency.find(nCurrentPolyID);
			if (adjIt != m_Adjacency.end())
			{
				for (int nNeighborID : adjIt->second)
				{
					Vector3 v3SharedV1, v3SharedV2;
					if (!GetSharedEdge(nCurrentPolyID, nNeighborID, v3SharedV1, v3SharedV2))
						continue;

					// QUANT_SCALE 정밀도로 엣지 일치 확인 (방향 무관)
					auto IsCloseXZ = [](const Vector3& A, const Vector3& B) -> bool
					{
						return std::abs(static_cast<int64>((A.x - B.x) * QUANT_SCALE)) == 0 &&
						       std::abs(static_cast<int64>((A.z - B.z) * QUANT_SCALE)) == 0;
					};

					if ((IsCloseXZ(v3SharedV1, v3ExitE1) && IsCloseXZ(v3SharedV2, v3ExitE2)) ||
					    (IsCloseXZ(v3SharedV1, v3ExitE2) && IsCloseXZ(v3SharedV2, v3ExitE1)))
					{
						nNextPolyID = nNeighborID;
						break;
					}
				}
			}

			// 경계 엣지(이웃 없음) → 벽에 막힘
			if (nNextPolyID < 0)
				return false;

			fCurrentT      = fMinT;
			nCurrentPolyID = nNextPolyID;
		}

		return false;  // 최대 반복 초과
	}


	// ─────────────────────────────────────────────────────────────────────────────
	// 디버그 렌더링용
	// ─────────────────────────────────────────────────────────────────────────────
	NavMeshDebugData NavMeshImpl::GetDebugLineVertices() const
	{
		NavMeshDebugData DebugData;

		// 1. 폴리곤 외곽선 (Polygon Edges)
		for (size_t tileIdx = 0; tileIdx < m_Tiles.size(); ++tileIdx)
		{
			const auto& Tile = m_Tiles[tileIdx];
			for (size_t polyIdx = 0; polyIdx < Tile.Polygons.size(); ++polyIdx)
			{
				const auto& Poly = Tile.Polygons[polyIdx];
				size_t nNumVerts = Poly.Indices.size();

				for (size_t i = 0; i < nNumVerts; ++i)
				{
					size_t nNextIdx = (i + 1) % nNumVerts;

					int idx1 = Poly.Indices[i];
					int idx2 = Poly.Indices[nNextIdx];

					// 인덱스 유효성 체크
					if (idx1 >= 0 && idx1 < (int)Tile.Vertices.size() &&
						idx2 >= 0 && idx2 < (int)Tile.Vertices.size())
					{
						// 폴리곤 외곽선 라인 추가
						DebugData.PolygonEdges.push_back(Tile.Vertices[idx1].v3Position);
						DebugData.PolygonEdges.push_back(Tile.Vertices[idx2].v3Position);
					}
				}
			}
		}

		// 2. 인접성 그래프 (Adjacency Graph)
		// 각 인접 관계에 대해 폴리곤 중심점을 연결하는 라인 생성
		std::unordered_set<uint64> ProcessedEdges;

		for (const auto& [polyID, neighbors] : m_Adjacency)
		{
			int nTileIdx = GetTileFromID(polyID);
			int nPolyIdx = GetPolyFromID(polyID);

			if (nTileIdx < 0 || nTileIdx >= (int)m_Tiles.size())
				continue;
			if (nPolyIdx < 0 || nPolyIdx >= (int)m_Tiles[nTileIdx].Polygons.size())
				continue;

			Vector3 v3CenterA = GetPolygonCenter(nTileIdx, nPolyIdx);

			for (int neighborID : neighbors)
			{
				// 중복 방지: (A->B)와 (B->A)를 한 번만 처리
				int nMinID = std::min(polyID, neighborID);
				int nMaxID = std::max(polyID, neighborID);
				uint64 nEdgeKey = ((uint64)nMinID << 32) | (uint64)nMaxID;

				if (ProcessedEdges.find(nEdgeKey) != ProcessedEdges.end())
					continue;

				ProcessedEdges.insert(nEdgeKey);

				int nNeighborTileIdx = GetTileFromID(neighborID);
				int nNeighborPolyIdx = GetPolyFromID(neighborID);

				if (nNeighborTileIdx < 0 || nNeighborTileIdx >= (int)m_Tiles.size())
					continue;
				if (nNeighborPolyIdx < 0 || nNeighborPolyIdx >= (int)m_Tiles[nNeighborTileIdx].Polygons.size())
					continue;

				Vector3 v3CenterB = GetPolygonCenter(nNeighborTileIdx, nNeighborPolyIdx);

				// 인접성 그래프 라인 추가
				DebugData.AdjacencyEdges.push_back(v3CenterA);
				DebugData.AdjacencyEdges.push_back(v3CenterB);
			}
		}

		return DebugData;
	}


	// ─────────────────────────────────────────────────────────────────────────
	// GetRandomPoint  : NavMesh 위의 랜덤 점 반환 (좀비 스폰 / GoalWander 용)
	//                   유효한 타일·폴리곤이 없으면 Vector3::Zero 반환
	// ─────────────────────────────────────────────────────────────────────────
	Vector3 NavMeshImpl::GetRandomPoint() const
	{
		if (m_Tiles.empty())
			return Vector3::Zero;

		// 유효한 타일(폴리곤 존재) 목록 수집
		std::vector<int> validTiles;
		validTiles.reserve(m_Tiles.size());
		for (int i = 0; i < static_cast<int>(m_Tiles.size()); ++i)
		{
			if (!m_Tiles[i].Polygons.empty())
				validTiles.push_back(i);
		}
		if (validTiles.empty())
			return Vector3::Zero;

		static std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int> tileDist(0, static_cast<int>(validTiles.size()) - 1);
		int tileIdx = validTiles[tileDist(rng)];

		const auto& tile = m_Tiles[tileIdx];
		std::uniform_int_distribution<int> polyDist(0, static_cast<int>(tile.Polygons.size()) - 1);
		const auto& poly = tile.Polygons[polyDist(rng)];

		if (poly.Indices.empty())
			return Vector3::Zero;

		// 폴리곤 무게중심 반환
		Vector3 centroid = Vector3::Zero;
		for (int idx : poly.Indices)
			centroid += tile.Vertices[idx].v3Position;
		centroid /= static_cast<float>(poly.Indices.size());

		return centroid;
	}
}
