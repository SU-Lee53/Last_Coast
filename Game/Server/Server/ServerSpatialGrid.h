#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// ServerSpatialGrid
//   클라이언트 SpatialWorld 의 경량 서버 버전.
//   정적 OBB 만 관리하며, Ray vs OBB 차폐 판정용.
// ─────────────────────────────────────────────────────────────────────────────

using StaticOBBID = unsigned int;
static constexpr StaticOBBID INVALID_STATIC_OBB_ID = 0xFFFFFFFF;

struct StaticOBBProxy
{
	StaticOBBID              id     = INVALID_STATIC_OBB_ID;
	DirectX::BoundingOrientedBox xmOBB  = {};
	DirectX::BoundingBox         xmAABB = {};
};

struct ServerGridCell
{
	DirectX::BoundingBox         xmAABB = {};
	std::vector<StaticOBBID>     proxyIDs;
};

struct ServerGridBuildDesc
{
	Vector2      v2OriginXZ   = {};
	Vector2      v2CellSizeXZ = { 500.f, 500.f };
	unsigned int nCellsX = 1;
	unsigned int nCellsZ = 1;
	float        fMinY = -10000.f;
	float        fMaxY = +10000.f;
};

struct ServerRayHitResult
{
	bool    bHit        = false;
	float   fDistance    = FLT_MAX;
	Vector3 v3HitPoint  = {};
	Vector3 v3HitNormal = {};
};

// ─────────────────────────────────────────────────────────────────────────────
class ServerSpatialGrid
{
public:
	bool LoadFromSceneFile(const std::string& strSceneJsonPath,
	                       const std::string& strModelDirectory);

	void BuildGrid(const ServerGridBuildDesc& desc);

	bool RayTestStatics(const Vector3& v3Origin,
	                    const Vector3& v3Direction,
	                    float fMaxDistance,
	                    ServerRayHitResult& outHit) const;

	bool IsLineOfSightBlocked(const Vector3& v3From,
	                          const Vector3& v3To) const;

	size_t GetProxyCount() const { return m_Proxies.size(); }

private:
	StaticOBBID AddProxy(const BoundingOrientedBox& xmOBB);

	void InsertProxyToGrid(StaticOBBID id);
	void GenerateCellBounds();

	int CellIndex(int x, int z) const;
	bool IsValidCell(int x, int z) const;

	struct CellCoord { int x = 0, z = 0; };
	struct CellRange { CellCoord min, max; };

	CellCoord WorldToCell(const Vector3& v3Pos) const;
	CellCoord ClampCell(CellCoord c) const;
	CellRange GetCellRangeFromAABB(const BoundingBox& xmAABB) const;

	bool IntersectRayGridXZ(const Vector3& v3Origin,
	                        const Vector3& v3Dir,
	                        float fMaxDist,
	                        float& outTEnter,
	                        float& outTExit) const;

	bool LoadMergedBounds(const std::string& strModelBinPath,
	                      BoundingBox& outAABB) const;

private:
	std::vector<StaticOBBProxy>    m_Proxies;
	std::vector<ServerGridCell>    m_GridCells;
	ServerGridBuildDesc            m_GridDesc = {};
	bool                           m_bGridBuilt = false;

	mutable unsigned int               m_nQueryStamp = 0;
	mutable std::vector<unsigned int>  m_ProxyQueryStamps;
};
