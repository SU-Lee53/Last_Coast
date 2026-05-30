#pragma once
#include "AI.h"
#include "NavMeshGraph.h"



namespace AIDLL
{

	struct NavMeshVertex
	{
		Vector3 v3Position;
	};

	struct NavMeshPolygon
	{
		std::vector<int> Indices;
		int nAreaType;
		int nFlags;
	};

	struct NavMeshTile
	{
		int nX;
		int nY; 
		int nLayer;
		Vector3 v3BoundsMin;
		Vector3 v3BoundsMax;
		std::vector<NavMeshVertex> Vertices;
		std::vector<NavMeshPolygon> Polygons;
	};

	// 두 정점 위치를 나타내는 엣지 키 (타일 경계 너머 인접도 처리)
	// 정밀도: 1mm 단위 정수화
	struct EdgeKey
	{
		int64 nX1;
		int64 nY1;
		int64 nZ1;
		int64 nX2;
		int64 nY2;
		int64 nZ2;

		bool operator==(const EdgeKey& o) const
		{
			return (nX1 == o.nX1 && nY1 == o.nY1 && nZ1 == o.nZ1 && nX2 == o.nX2 && nY2 == o.nY2 && nZ2 == o.nZ2) ||
				(nX1 == o.nX2 && nY1 == o.nY2 && nZ1 == o.nZ2 && nX2 == o.nX1 && nY2 == o.nY1 && nZ2 == o.nZ1);
		}
	};

	struct EdgeKeyHash
	{
		size_t operator()(const EdgeKey& k) const
		{
			// 두 끝점을 사전순으로 정규화하여 방향에 무관한 해시 생성
			int64 nAx1 = k.nX1, ay1 = k.nY1, az1 = k.nZ1;
			int64 nAx2 = k.nX2, ay2 = k.nY2, az2 = k.nZ2;
			if (std::tie(nAx1, ay1, az1) > std::tie(nAx2, ay2, az2))
			{
				std::swap(nAx1, nAx2);
				std::swap(ay1, ay2);
				std::swap(az1, az2);
			}
			size_t h = 0;
			auto hc = [&](int64 v)
				{
					h ^= std::hash<int64>{}(v)+0x9e3779b9ull + (h << 6) + (h >> 2);
				};
			hc(nAx1); hc(ay1); hc(az1);
			hc(nAx2); hc(ay2); hc(az2);
			return h;
		}
	};

	// 공유 정점과 폴리곤 A에서의 인덱스 위치를 함께 저장
	struct SharedVertex
	{
		Vector3 v3Pos;
		int nIndexInPolyA;  // PolyA.Indices 배열에서의 위치 (순서)
	};

	constexpr float QUANT_SCALE = 1000.0f; // 1mm 정밀도

	class NavMeshImpl : public INavMesh
	{
	public:
		NavMeshImpl();
		virtual ~NavMeshImpl();

		// INavMesh 구현
		virtual bool LoadFromJson(const std::string& strFileName) override;
		virtual bool IsPointOnNavMesh(const Vector3& point) const override;
		virtual Vector3 GetNearestPointOnNavMesh(const Vector3& point) const override;
		virtual Vector3 GetRandomPoint() const override;
		virtual bool IsLineOfSightClear(const Vector3& v3From, const Vector3& To) const override;
		//virtual bool FindPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath) override;
		virtual NavMeshConfig GetConfig() const override { return m_Config; }
		virtual int GetTileCount() const override { return static_cast<int>(m_Tiles.size()); }
		virtual NavMeshDebugData GetDebugLineVertices() const override;

		// ── AIPathPlanner 에서 사용하는 공개 헬퍼 ─────────────────────────────

		// point 를 포함하는 폴리곤의 전역 PolyID 반환 (없으면 -1)
		int FindPolygonContaining(const Vector3& point) const;

		// 폴리곤 그래프를 Dense-indexed NavMeshGraph 로 빌드해 반환
		// NavMesh 로드 후 AIManagerImpl 에서 한 번 호출한다.
		std::unique_ptr<NavMeshGraph> BuildGraph() const;

		// 두 인접 폴리곤의 공유 엣지 두 정점을 반환 (String Pulling 용)
		// 반환값: 공유 엣지가 존재하면 true, outV1/outV2 에 정점 좌표 저장
		bool GetSharedEdge(int globalPolyID_A, int globalPolyID_B,
		                   Vector3& outV1, Vector3& outV2) const;

		// ── 디버그 렌더링용 ──────────────────────────────────────────────────
		const std::vector<NavMeshTile>& GetTiles() const { return m_Tiles; }

	private:
		NavMeshConfig m_Config;
		std::vector<NavMeshTile> m_Tiles;

		// 폴리곤 인접 그래프: 전역 PolyID -> 인접한 전역 PolyID 목록
		// 전역 PolyID = tileIdx * 100000 + polyIdx
		std::unordered_map<int, std::vector<int>> m_Adjacency;

		// 메인 연결 컴포넌트(가장 큰 섬)에 속한 폴리곤 ID 셋
		// 고립된 섬 폴리곤은 여기에 없음 → FindPolygonContaining 등에서 제외
		std::unordered_set<int> m_MainComponentPolys;

		// 인접성 구축 (LoadFromJson 이후 호출)
		void BuildAdjacency();

		// 전역 PolyID 인코딩/디코딩
		int  MakePolyID(int tileIdx, int polyIdx) const { return tileIdx * 100000 + polyIdx; }
		int  GetTileFromID(int id) const { return id / 100000; }
		int  GetPolyFromID(int id) const { return id % 100000; }

		// 폴리곤 무게중심 반환
		Vector3 GetPolygonCenter(int tileIdx, int polyIdx) const;

		bool IsPointInPolygon(const Vector3& point, const NavMeshPolygon& poly, const NavMeshTile& tile) const;
		float DistanceToPolygon(const Vector3& point, const NavMeshPolygon& poly, const NavMeshTile& tile) const;

		static Vector3 ClosestPointOnSegment(const Vector3& p, const Vector3& a, const Vector3& b);

		// XZ 평면 선분 교차 판정 (IsLineOfSightClear 전용 헬퍼)
		// fOutT: ray (P1→P2) 위의 교차 파라미터, fOutS: edge (P3→P4) 위의 교차 파라미터
		static bool SegmentIntersect2D(const Vector3& P1, const Vector3& P2,
		                               const Vector3& P3, const Vector3& P4,
		                               float& OutT, float& OutS);
		std::pair<float, Vector3> ClosestPointOnPolygon(const Vector3& p, const NavMeshPolygon& poly,const NavMeshTile& tile) const;

		inline static std::string g_strAIBasePath = "../Resources/NavMesh";
	};

}

