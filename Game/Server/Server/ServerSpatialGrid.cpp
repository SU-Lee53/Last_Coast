#include "pch.h"
#include "ServerSpatialGrid.h"

#define JSON_HAS_RANGES 0
#include <Includes/nlohmann_json/json.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// 모델 BSON 에서 MergedBounds 읽기
// ─────────────────────────────────────────────────────────────────────────────
bool ServerSpatialGrid::LoadMergedBounds(const std::string& strModelBinPath,
                                         BoundingBox& outAABB) const
{
	std::ifstream in(strModelBinPath, std::ios::binary);
	if (!in) {
		// std::cout << "  [LoadMergedBounds] File not found: " << strModelBinPath << "\n";
		return false;
	}

	std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in)),
	                          std::istreambuf_iterator<char>());

	nlohmann::json j = nlohmann::json::from_bson(buf);

	if (!j.contains("MergedBounds")) {
		// std::cout << "  [LoadMergedBounds] No MergedBounds in: " << strModelBinPath << "\n";
		return false;
	}

	const auto& mb = j["MergedBounds"];
	if (!mb.contains("Center") || !mb.contains("Extents"))
		return false;

	const auto& c = mb["Center"];
	const auto& e = mb["Extents"];

	outAABB.Center  = XMFLOAT3{ c[0].get<float>(), c[1].get<float>(), c[2].get<float>() };
	outAABB.Extents = XMFLOAT3{ e[0].get<float>(), e[1].get<float>(), e[2].get<float>() };

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// 씬 JSON + 모델 파일 로드 → OBB 등록 → 그리드 빌드
// ─────────────────────────────────────────────────────────────────────────────
bool ServerSpatialGrid::LoadFromSceneFile(const std::string& strSceneJsonPath,
                                          const std::string& strModelDirectory)
{
	std::ifstream sceneIn(strSceneJsonPath);
	if (!sceneIn) {
		// std::cout << "[ServerSpatialGrid] Failed to open scene: " << strSceneJsonPath << "\n";
		return false;
	}

	nlohmann::json sceneJson = nlohmann::json::parse(sceneIn);
	if (!sceneJson.contains("StaticMeshActors"))
		return false;

	std::unordered_map<std::string, BoundingBox> boundsCache;

	Vector3 v3SceneMin = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 v3SceneMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& actor : sceneJson["StaticMeshActors"])
	{
		std::string strMeshName = actor["MeshName"].get<std::string>();

		BoundingBox modelAABB;
		auto it = boundsCache.find(strMeshName);
		if (it != boundsCache.end()) {
			modelAABB = it->second;
		}
		else {
			std::string strBinPath = strModelDirectory + "/" + strMeshName + ".bin";
			if (!LoadMergedBounds(strBinPath, modelAABB))
				continue;
			boundsCache[strMeshName] = modelAABB;
		}

		const auto& wm = actor["Transform"]["WorldMatrix"];
		XMFLOAT4X4 xmf4x4World;
		for (int i = 0; i < 16; ++i)
			reinterpret_cast<float*>(&xmf4x4World)[i] = wm[i].get<float>();

		XMMATRIX xmmtxWorld = XMLoadFloat4x4(&xmf4x4World);

		BoundingOrientedBox modelOBB;
		BoundingOrientedBox::CreateFromBoundingBox(modelOBB, modelAABB);

		BoundingOrientedBox worldOBB;
		modelOBB.Transform(worldOBB, xmmtxWorld);

		StaticOBBID id = AddProxy(worldOBB);

		const BoundingBox& proxyAABB = m_Proxies[id].xmAABB;
		Vector3 pMin = Vector3(proxyAABB.Center) - Vector3(proxyAABB.Extents);
		Vector3 pMax = Vector3(proxyAABB.Center) + Vector3(proxyAABB.Extents);

		v3SceneMin = Vector3::Min(v3SceneMin, pMin);
		v3SceneMax = Vector3::Max(v3SceneMax, pMax);
	}

	if (m_Proxies.empty()) {
		// std::cout << "[ServerSpatialGrid] No static objects loaded.\n";
		return false;
	}

	constexpr float CELL_SIZE = 500.f;

	ServerGridBuildDesc desc;
	desc.v2OriginXZ  = Vector2{ v3SceneMin.x, v3SceneMin.z };
	desc.v2CellSizeXZ = Vector2{ CELL_SIZE, CELL_SIZE };
	desc.nCellsX = std::max(1u, static_cast<unsigned int>(
		std::ceil((v3SceneMax.x - v3SceneMin.x) / CELL_SIZE)));
	desc.nCellsZ = std::max(1u, static_cast<unsigned int>(
		std::ceil((v3SceneMax.z - v3SceneMin.z) / CELL_SIZE)));
	desc.fMinY = v3SceneMin.y;
	desc.fMaxY = v3SceneMax.y;

	BuildGrid(desc);

	// std::cout << "[ServerSpatialGrid] Loaded " << m_Proxies.size()
	//          << " static OBBs, grid " << desc.nCellsX << "x" << desc.nCellsZ << "\n";

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// 프록시 등록
// ─────────────────────────────────────────────────────────────────────────────
StaticOBBID ServerSpatialGrid::AddProxy(const BoundingOrientedBox& xmOBB)
{
	StaticOBBProxy proxy;
	proxy.id   = static_cast<StaticOBBID>(m_Proxies.size());
	proxy.xmOBB = xmOBB;

	XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
	xmOBB.GetCorners(corners);
	BoundingBox::CreateFromPoints(proxy.xmAABB,
		BoundingOrientedBox::CORNER_COUNT, corners, sizeof(XMFLOAT3));

	m_Proxies.push_back(proxy);
	m_ProxyQueryStamps.push_back(0);

	return proxy.id;
}

// ─────────────────────────────────────────────────────────────────────────────
// 그리드 빌드
// ─────────────────────────────────────────────────────────────────────────────
void ServerSpatialGrid::BuildGrid(const ServerGridBuildDesc& desc)
{
	m_GridDesc = desc;
	m_GridCells.clear();
	m_GridCells.resize(static_cast<size_t>(desc.nCellsX) * desc.nCellsZ);

	GenerateCellBounds();

	for (size_t i = 0; i < m_Proxies.size(); ++i)
		InsertProxyToGrid(static_cast<StaticOBBID>(i));

	m_bGridBuilt = true;
}

void ServerSpatialGrid::GenerateCellBounds()
{
	for (unsigned int z = 0; z < m_GridDesc.nCellsZ; ++z) {
		for (unsigned int x = 0; x < m_GridDesc.nCellsX; ++x) {
			float fMinX = m_GridDesc.v2OriginXZ.x + static_cast<float>(x) * m_GridDesc.v2CellSizeXZ.x;
			float fMinZ = m_GridDesc.v2OriginXZ.y + static_cast<float>(z) * m_GridDesc.v2CellSizeXZ.y;

			Vector3 vMin{ fMinX, m_GridDesc.fMinY, fMinZ };
			Vector3 vMax{ fMinX + m_GridDesc.v2CellSizeXZ.x, m_GridDesc.fMaxY, fMinZ + m_GridDesc.v2CellSizeXZ.y };

			BoundingBox::CreateFromPoints(
				m_GridCells[CellIndex(x, z)].xmAABB, vMin, vMax);
		}
	}
}

void ServerSpatialGrid::InsertProxyToGrid(StaticOBBID id)
{
	CellRange range = GetCellRangeFromAABB(m_Proxies[id].xmAABB);
	for (int z = range.min.z; z <= range.max.z; ++z)
		for (int x = range.min.x; x <= range.max.x; ++x)
			if (IsValidCell(x, z))
				m_GridCells[CellIndex(x, z)].proxyIDs.push_back(id);
}

// ─────────────────────────────────────────────────────────────────────────────
// 그리드 좌표 유틸
// ─────────────────────────────────────────────────────────────────────────────
int ServerSpatialGrid::CellIndex(int x, int z) const
{
	return z * static_cast<int>(m_GridDesc.nCellsX) + x;
}

bool ServerSpatialGrid::IsValidCell(int x, int z) const
{
	return x >= 0 && z >= 0 &&
	       x < static_cast<int>(m_GridDesc.nCellsX) &&
	       z < static_cast<int>(m_GridDesc.nCellsZ);
}

ServerSpatialGrid::CellCoord ServerSpatialGrid::WorldToCell(const Vector3& v3Pos) const
{
	float fx = (v3Pos.x - m_GridDesc.v2OriginXZ.x) / m_GridDesc.v2CellSizeXZ.x;
	float fz = (v3Pos.z - m_GridDesc.v2OriginXZ.y) / m_GridDesc.v2CellSizeXZ.y;
	return { static_cast<int>(std::floor(fx)), static_cast<int>(std::floor(fz)) };
}

ServerSpatialGrid::CellCoord ServerSpatialGrid::ClampCell(CellCoord c) const
{
	c.x = std::clamp(c.x, 0, static_cast<int>(m_GridDesc.nCellsX) - 1);
	c.z = std::clamp(c.z, 0, static_cast<int>(m_GridDesc.nCellsZ) - 1);
	return c;
}

ServerSpatialGrid::CellRange ServerSpatialGrid::GetCellRangeFromAABB(const BoundingBox& xmAABB) const
{
	Vector3 vMin = Vector3(xmAABB.Center) - Vector3(xmAABB.Extents);
	Vector3 vMax = Vector3(xmAABB.Center) + Vector3(xmAABB.Extents);
	return { ClampCell(WorldToCell(vMin)), ClampCell(WorldToCell(vMax)) };
}

// ─────────────────────────────────────────────────────────────────────────────
// DDA 레이-그리드 교차
// ─────────────────────────────────────────────────────────────────────────────
bool ServerSpatialGrid::IntersectRayGridXZ(const Vector3& v3Origin,
                                           const Vector3& v3Dir,
                                           float fMaxDist,
                                           float& outTEnter,
                                           float& outTExit) const
{
	float minX = m_GridDesc.v2OriginXZ.x;
	float minZ = m_GridDesc.v2OriginXZ.y;
	float maxX = minX + m_GridDesc.v2CellSizeXZ.x * static_cast<float>(m_GridDesc.nCellsX);
	float maxZ = minZ + m_GridDesc.v2CellSizeXZ.y * static_cast<float>(m_GridDesc.nCellsZ);

	float tEnter = 0.f, tExit = fMaxDist;
	constexpr float EPS = 1e-6f;

	auto updateSlab = [&](float origin, float dir, float slabMin, float slabMax) -> bool {
		if (std::abs(dir) < EPS)
			return origin >= slabMin && origin <= slabMax;
		float t1 = (slabMin - origin) / dir;
		float t2 = (slabMax - origin) / dir;
		if (t1 > t2) std::swap(t1, t2);
		tEnter = std::max(tEnter, t1);
		tExit  = std::min(tExit, t2);
		return tEnter <= tExit;
	};

	if (!updateSlab(v3Origin.x, v3Dir.x, minX, maxX)) return false;
	if (!updateSlab(v3Origin.z, v3Dir.z, minZ, maxZ)) return false;
	if (tExit < 0.f) return false;

	tEnter = std::max(0.f, tEnter);
	tExit  = std::min(fMaxDist, tExit);
	if (tEnter > tExit) return false;

	outTEnter = tEnter;
	outTExit  = tExit;
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// 레이 vs 정적 OBB 쿼리 (DDA 그리드 순회)
// ─────────────────────────────────────────────────────────────────────────────
bool ServerSpatialGrid::RayTestStatics(const Vector3& v3Origin,
                                       const Vector3& v3Direction,
                                       float fMaxDistance,
                                       ServerRayHitResult& outHit) const
{
	outHit = {};

	if (!m_bGridBuilt || m_Proxies.empty())
		return false;

	Vector3 v3Dir = v3Direction;
	if (v3Dir.LengthSquared() <= 1e-8f)
		return false;
	v3Dir.Normalize();

	float tEnter = 0.f, tExit = 0.f;
	if (!IntersectRayGridXZ(v3Origin, v3Dir, fMaxDistance, tEnter, tExit))
		return false;

	XMVECTOR xmRayOrigin = XMLoadFloat3(&v3Origin);
	XMVECTOR xmRayDir    = XMLoadFloat3(&v3Dir);

	++m_nQueryStamp;
	if (m_nQueryStamp == 0) {
		std::fill(m_ProxyQueryStamps.begin(), m_ProxyQueryStamps.end(), 0);
		m_nQueryStamp = 1;
	}

	Vector3 v3Start = v3Origin + v3Dir * tEnter;
	CellCoord cell = ClampCell(WorldToCell(v3Start));

	int stepX = v3Dir.x > 0.f ? 1 : (v3Dir.x < 0.f ? -1 : 0);
	int stepZ = v3Dir.z > 0.f ? 1 : (v3Dir.z < 0.f ? -1 : 0);

	constexpr float INF = std::numeric_limits<float>::infinity();
	constexpr float EPS = 1e-6f;

	float tMaxX = INF, tMaxZ = INF;
	float tDeltaX = INF, tDeltaZ = INF;

	if (stepX != 0 && std::abs(v3Dir.x) > EPS) {
		float nextX = m_GridDesc.v2OriginXZ.x +
			static_cast<float>(cell.x + (stepX > 0 ? 1 : 0)) * m_GridDesc.v2CellSizeXZ.x;
		tMaxX   = (nextX - v3Origin.x) / v3Dir.x;
		tDeltaX = m_GridDesc.v2CellSizeXZ.x / std::abs(v3Dir.x);
	}

	if (stepZ != 0 && std::abs(v3Dir.z) > EPS) {
		float nextZ = m_GridDesc.v2OriginXZ.y +
			static_cast<float>(cell.z + (stepZ > 0 ? 1 : 0)) * m_GridDesc.v2CellSizeXZ.y;
		tMaxZ   = (nextZ - v3Origin.z) / v3Dir.z;
		tDeltaZ = m_GridDesc.v2CellSizeXZ.y / std::abs(v3Dir.z);
	}

	float fBestDist = fMaxDistance;
	bool  bHit = false;
	float tCurrent = tEnter;

	while (IsValidCell(cell.x, cell.z) && tCurrent <= tExit)
	{
		const ServerGridCell& gridCell = m_GridCells[CellIndex(cell.x, cell.z)];

		for (StaticOBBID proxyID : gridCell.proxyIDs)
		{
			if (m_ProxyQueryStamps[proxyID] == m_nQueryStamp)
				continue;
			m_ProxyQueryStamps[proxyID] = m_nQueryStamp;

			const StaticOBBProxy& proxyRef = m_Proxies[proxyID];

			float fAABBDist = 0.f;
			if (!proxyRef.xmAABB.Intersects(xmRayOrigin, xmRayDir, fAABBDist))
				continue;
			if (fAABBDist > fBestDist)
				continue;

			float fOBBDist = 0.f;
			if (!proxyRef.xmOBB.Intersects(xmRayOrigin, xmRayDir, fOBBDist))
				continue;
			if (fOBBDist < 0.f || fOBBDist > fBestDist)
				continue;

			fBestDist = fOBBDist;
			bHit = true;

			outHit.bHit       = true;
			outHit.fDistance   = fOBBDist;
			outHit.v3HitPoint = v3Origin + v3Dir * fOBBDist;
			outHit.v3HitNormal = -v3Dir;
		}

		if (tMaxX < tMaxZ) {
			cell.x += stepX;
			tCurrent = tMaxX;
			tMaxX += tDeltaX;
		}
		else if (tMaxZ < tMaxX) {
			cell.z += stepZ;
			tCurrent = tMaxZ;
			tMaxZ += tDeltaZ;
		}
		else {
			cell.x += stepX;
			cell.z += stepZ;
			tCurrent = tMaxX;
			tMaxX += tDeltaX;
			tMaxZ += tDeltaZ;
		}
	}

	return bHit;
}

// ─────────────────────────────────────────────────────────────────────────────
// 두 점 사이 정적 차폐 여부
// ─────────────────────────────────────────────────────────────────────────────
bool ServerSpatialGrid::IsLineOfSightBlocked(const Vector3& v3From,
                                             const Vector3& v3To) const
{
	Vector3 v3Dir = v3To - v3From;
	float fDist = v3Dir.Length();
	if (fDist < 1e-4f)
		return false;

	v3Dir /= fDist;

	ServerRayHitResult hit;
	if (!RayTestStatics(v3From, v3Dir, fDist, hit))
		return false;

	return hit.fDistance < fDist;
}
