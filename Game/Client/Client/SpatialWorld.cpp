#include "pch.h"
#include "SpatialWorld.h"

void SpatialWorld::Clear()
{
	m_Proxies.clear();

	m_ObjectToProxy.clear();
	m_TerrainToProxy.clear();
	m_LightToProxy.clear();

	m_StaticGridCells.clear();
	m_bStaticGridBuilt = false;

	m_DynamicGridCells.clear();
	m_bDynamicGridBuilt = false;

	m_ProxyQueryStamps.clear();
	m_unCurrentQueryStamp = 0;

	m_StaticGridDesc = {};
}

SpatialProxyID SpatialWorld::RegisterObjectImpl(IGameObject* pObject, uint32 unLayerMask, bool bDynamic, std::type_index objectType)
{
	if (!pObject) {
		return INVALID_SPATIAL_PROXY_ID;
	}

	if (m_ObjectToProxy.contains(pObject)) {
		return m_ObjectToProxy[pObject];
	}

	SpatialProxy proxy{};
	proxy.id = static_cast<SpatialProxyID>(m_Proxies.size());
	proxy.type = SPATIAL_PROXY_TYPE::OBJECT;
	proxy.pObject = pObject;
	proxy.objectTypeID = objectType;

	proxy.unLayerMask =
		unLayerMask |
		(bDynamic ? SPATIAL_DYNAMIC : SPATIAL_STATIC);

	proxy.bDynamic = bDynamic;
	proxy.bAlive = true;
	proxy.bDirty = true;

	UpdateObjectBounds(proxy);

	m_Proxies.push_back(proxy);
	m_ProxyQueryStamps.push_back(0);

	m_ObjectToProxy[pObject] = proxy.id;

	return proxy.id;
}

SpatialProxyID SpatialWorld::RegisterTerrain(TerrainComponent* pTerrainComponent, uint32 unLayerMask)
{
	if (!pTerrainComponent) {
		return INVALID_SPATIAL_PROXY_ID;
	}

	if (m_TerrainToProxy.contains(pTerrainComponent)) {
		return m_TerrainToProxy[pTerrainComponent];
	}

	SpatialProxy proxy{};
	proxy.id = static_cast<SpatialProxyID>(m_Proxies.size());
	proxy.type = SPATIAL_PROXY_TYPE::TERRAIN;
	proxy.pTerrainComponent = pTerrainComponent;
	proxy.unLayerMask = unLayerMask | SPATIAL_STATIC;
	proxy.bDynamic = false;
	proxy.bAlive = true;
	proxy.bDirty = true;

	UpdateTerrainBounds(proxy);

	m_Proxies.push_back(proxy);
	m_ProxyQueryStamps.push_back(0);

	m_TerrainToProxy[pTerrainComponent] = proxy.id;

	return proxy.id;
}

SpatialProxyID SpatialWorld::RegisterLight(Light* pLight, uint32 unLayerMask, bool bDynamic)
{
	if (!pLight) {
		return INVALID_SPATIAL_PROXY_ID;
	}

	if (m_LightToProxy.contains(pLight)) {
		return m_LightToProxy[pLight];
	}

	SpatialProxy proxy{};
	proxy.id = static_cast<SpatialProxyID>(m_Proxies.size());
	proxy.type = SPATIAL_PROXY_TYPE::LIGHT;
	proxy.pLight = pLight;
	proxy.unLayerMask = unLayerMask | SPATIAL_LIGHT | (bDynamic ? SPATIAL_DYNAMIC : SPATIAL_STATIC);
	proxy.bDynamic = bDynamic;
	proxy.bAlive = true;
	proxy.bDirty = true;

	UpdateLightBounds(proxy);

	m_Proxies.push_back(proxy);
	m_ProxyQueryStamps.push_back(0);

	m_LightToProxy[pLight] = proxy.id;

	return proxy.id;
}

void SpatialWorld::UnregisterObject(IGameObject* pObject)
{
	auto it = m_ObjectToProxy.find(pObject);
	if (it == m_ObjectToProxy.end()) {
		return;
	}

	SpatialProxyID id = it->second;
	if (IsValidProxyID(id)) {
		m_Proxies[id].bAlive = false;
		m_Proxies[id].pObject = nullptr;
	}

	m_ObjectToProxy.erase(it);
}

void SpatialWorld::UnregisterTerrain(TerrainComponent* pTerrainComponent)
{
	auto it = m_TerrainToProxy.find(pTerrainComponent);
	if (it == m_TerrainToProxy.end()) {
		return;
	}

	SpatialProxyID id = it->second;
	if (IsValidProxyID(id)) {
		m_Proxies[id].bAlive = false;
		m_Proxies[id].pTerrainComponent = nullptr;
	}

	m_TerrainToProxy.erase(it);
}

void SpatialWorld::UnregisterLight(Light* pLight)
{
	auto it = m_LightToProxy.find(pLight);
	if (it == m_LightToProxy.end()) {
		return;
	}

	SpatialProxyID id = it->second;
	if (IsValidProxyID(id)) {
		m_Proxies[id].bAlive = false;
		m_Proxies[id].pLight = nullptr;
	}

	m_LightToProxy.erase(it);
}

void SpatialWorld::BuildStaticGrid(const SpatialGridBuildDesc& desc)
{
	m_StaticGridDesc = desc;

	const uint32 cellsX = std::max(1u, desc.xmui2NumCellsXZ.x);
	const uint32 cellsZ = std::max(1u, desc.xmui2NumCellsXZ.y);

	m_StaticGridDesc.xmui2NumCellsXZ = XMUINT2{ cellsX, cellsZ };

	m_StaticGridCells.clear();
	m_StaticGridCells.resize(static_cast<size_t>(cellsX) * static_cast<size_t>(cellsZ));

	GenerateStaticGridCellBounds();

	for (const SpatialProxy& proxy : m_Proxies) {
		if (!proxy.bAlive) {
			continue;
		}

		if (proxy.bDynamic) {
			continue;
		}

		InsertProxyToStaticGrid(proxy.id);
	}

	m_bStaticGridBuilt = true;

	RebuildDynamicGrid();
}

void SpatialWorld::MarkObjectDirty(IGameObject* pObject)
{
	auto it = m_ObjectToProxy.find(pObject);
	if (it == m_ObjectToProxy.end()) {
		return;
	}

	if (IsValidProxyID(it->second)) {
		m_Proxies[it->second].bDirty = true;
	}
}

void SpatialWorld::UpdateDirtyProxies()
{
	bool bHasDynamicProxy = false;

	for (SpatialProxy& proxy : m_Proxies) {
		if (!proxy.bAlive) {
			continue;
		}

		if (proxy.bDynamic) {
			bHasDynamicProxy = true;
		}

		// Static -> Update when dirty
		// Dynamic -> Update every frames
		if (!proxy.bDynamic && !proxy.bDirty) {
			continue;
		}

		switch (proxy.type) {
		case SPATIAL_PROXY_TYPE::OBJECT:
			UpdateObjectBounds(proxy);
			break;

		case SPATIAL_PROXY_TYPE::TERRAIN:
			UpdateTerrainBounds(proxy);
			break;

		case SPATIAL_PROXY_TYPE::LIGHT:
			UpdateLightBounds(proxy);
			break;

		default:
			break;
		}

		proxy.bDirty = false;
	}

	if (bHasDynamicProxy && m_bStaticGridBuilt) {
		RebuildDynamicGrid();
	}
}

SpatialQueryResult SpatialWorld::QueryFrustumLinear(const BoundingFrustum& xmFrustumWorld, const SpatialQueryDesc& desc) const
{
	SpatialQueryResult result{};
	result.Reserve(64);

	for (const SpatialProxy& proxy : m_Proxies) {
		if (!ShouldTestProxy(proxy, desc)) {
			continue;
		}

		if (!xmFrustumWorld.Intersects(proxy.xmAABB)) {
			continue;
		}

		PushProxyToResult(proxy, result);
	}

	return result;
}

SpatialQueryResult SpatialWorld::QueryFrustum(const BoundingFrustum& xmFrustumWorld, const SpatialQueryDesc& desc) const
{
	if (!m_bStaticGridBuilt) {
		return QueryFrustumLinear(xmFrustumWorld, desc);
	}

	SpatialQueryResult result{};
	result.Reserve(64);

	const uint32 queryStamp = BeginSpatialQuery();

	BoundingBox frustumAABB = MakeAABBFromFrustum(xmFrustumWorld);
	SpatialCellRange range = GetStaticGridCellRangeFromAABB(frustumAABB);

	// 1. Static Grid Query
	if (desc.bIncludeStatic) {
		for (int32 z = range.min.z; z <= range.max.z; ++z) {
			for (int32 x = range.min.x; x <= range.max.x; ++x) {
				if (!IsValidStaticGridCell(x, z)) {
					continue;
				}

				const SpatialGridCell& cell =
					m_StaticGridCells[StaticGridCellToIndex(x, z)];

				if (!xmFrustumWorld.Intersects(cell.xmAABB)) {
					continue;
				}

				for (SpatialProxyID proxyID : cell.proxyIDs) {
					if (!TryMarkProxyVisited(proxyID, queryStamp)) {
						continue;
					}

					if (proxyID >= m_Proxies.size()) {
						continue;
					}

					const SpatialProxy& proxy = m_Proxies[proxyID];

					if (!ShouldTestProxy(proxy, desc)) {
						continue;
					}

					if (!xmFrustumWorld.Intersects(proxy.xmAABB)) {
						continue;
					}

					PushProxyToResult(proxy, result);
				}
			}
		}
	}

	// 2. Dynamic Grid Query
	if (desc.bIncludeDynamic && m_bDynamicGridBuilt) {
		for (int32 z = range.min.z; z <= range.max.z; ++z) {
			for (int32 x = range.min.x; x <= range.max.x; ++x) {
				if (!IsValidStaticGridCell(x, z)) {
					continue;
				}

				const int32 cellIndex = StaticGridCellToIndex(x, z);

				// DynamicGridCell 자체에는 AABB를 안 넣고 있으므로,
				// 셀 bounds 검사는 StaticGridCell의 xmAABB를 사용한다.
				const SpatialGridCell& staticCell = m_StaticGridCells[cellIndex];

				if (!xmFrustumWorld.Intersects(staticCell.xmAABB)) {
					continue;
				}

				const SpatialGridCell& dynamicCell = m_DynamicGridCells[cellIndex];

				for (SpatialProxyID proxyID : dynamicCell.proxyIDs) {
					if (!TryMarkProxyVisited(proxyID, queryStamp)) {
						continue;
					}

					if (proxyID >= m_Proxies.size()) {
						continue;
					}

					const SpatialProxy& proxy = m_Proxies[proxyID];

					if (!ShouldTestProxy(proxy, desc)) {
						continue;
					}

					if (!xmFrustumWorld.Intersects(proxy.xmAABB)) {
						continue;
					}

					PushProxyToResult(proxy, result);
				}
			}
		}
	}

	return result;
}

SpatialQueryResult SpatialWorld::QueryAABB(const BoundingBox& xmAABB, const SpatialQueryDesc& desc) const
{
	BoundingBox queryAABB = xmAABB;
	queryAABB.Extents.x += desc.v3AABBInflation.x;
	queryAABB.Extents.y += desc.v3AABBInflation.y;
	queryAABB.Extents.z += desc.v3AABBInflation.z;

	if (!m_bStaticGridBuilt) {
		SpatialQueryResult result{};
		result.Reserve(64);

		for (const SpatialProxy& proxy : m_Proxies) {
			if (!ShouldTestProxy(proxy, desc)) {
				continue;
			}

			if (!queryAABB.Intersects(proxy.xmAABB)) {
				continue;
			}

			PushProxyToResult(proxy, result);
		}

		return result;
	}

	SpatialQueryResult result{};
	result.Reserve(64);

	const uint32 queryStamp = BeginSpatialQuery();
	SpatialCellRange range = GetStaticGridCellRangeFromAABB(queryAABB);

	// Static Grid Query
	if (desc.bIncludeStatic) {
		for (int32 z = range.min.z; z <= range.max.z; ++z) {
			for (int32 x = range.min.x; x <= range.max.x; ++x) {
				if (!IsValidStaticGridCell(x, z)) {
					continue;
				}

				const SpatialGridCell& cell =
					m_StaticGridCells[StaticGridCellToIndex(x, z)];

				for (SpatialProxyID proxyID : cell.proxyIDs) {
					if (!TryMarkProxyVisited(proxyID, queryStamp)) {
						continue;
					}

					if (proxyID >= m_Proxies.size()) {
						continue;
					}

					const SpatialProxy& proxy = m_Proxies[proxyID];

					if (!ShouldTestProxy(proxy, desc)) {
						continue;
					}

					// 여기서는 정확한 xmAABB가 아니라 부풀린 queryAABB를 써야 함.
					if (!queryAABB.Intersects(proxy.xmAABB)) {
						continue;
					}

					PushProxyToResult(proxy, result);
				}
			}
		}
	}

	// Dynamic Grid Query
	if (desc.bIncludeDynamic && m_bDynamicGridBuilt) {
		for (int32 z = range.min.z; z <= range.max.z; ++z) {
			for (int32 x = range.min.x; x <= range.max.x; ++x) {
				if (!IsValidStaticGridCell(x, z)) {
					continue;
				}

				const SpatialGridCell& cell =
					m_DynamicGridCells[StaticGridCellToIndex(x, z)];

				for (SpatialProxyID proxyID : cell.proxyIDs) {
					if (!TryMarkProxyVisited(proxyID, queryStamp)) {
						continue;
					}

					if (proxyID >= m_Proxies.size()) {
						continue;
					}

					const SpatialProxy& proxy = m_Proxies[proxyID];

					if (!ShouldTestProxy(proxy, desc)) {
						continue;
					}

					if (!queryAABB.Intersects(proxy.xmAABB)) {
						continue;
					}

					PushProxyToResult(proxy, result);
				}
			}
		}
	}

	return result;
}

SpatialQueryResult SpatialWorld::QuerySphere(const BoundingSphere& xmSphere, const SpatialQueryDesc& desc) const
{
	SpatialQueryResult result{};
	result.Reserve(64);

	for (const SpatialProxy& proxy : m_Proxies) {
		if (!ShouldTestProxy(proxy, desc)) {
			continue;
		}

		if (!xmSphere.Intersects(proxy.xmAABB)) {
			continue;
		}

		PushProxyToResult(proxy, result);
	}

	return result;
}

SpatialRayQueryResult SpatialWorld::QueryRay(const SpatialRayQueryDesc& desc) const
{
	SpatialRayQueryResult result{};
	result.Reserve(32);

	if (!m_bStaticGridBuilt) {
		return result;
	}

	Vector3 v3Dir = desc.v3Direction;
	if (v3Dir.LengthSquared() <= 1e-8f || desc.fMaxDistance <= 0.f) {
		return result;
	}

	v3Dir.Normalize();

	float tEnter = 0.f;
	float tExit = 0.f;

	const bool bIntersectsGrid =
		IntersectRayGridXZ(
			desc.v3Origin,
			v3Dir,
			desc.fMaxDistance,
			tEnter,
			tExit
		);

	if (!bIntersectsGrid) {
		return result;
	}

	const XMVECTOR xmRayOrigin = XMLoadFloat3(&desc.v3Origin);
	const XMVECTOR xmRayDir = XMLoadFloat3(&v3Dir);

	const uint32 queryStamp = BeginSpatialQuery();

	const Vector3 v3Start =
		desc.v3Origin + v3Dir * tEnter;

	SpatialCellCoord cell =
		ClampStaticGridCell(WorldToStaticGridCellXZ(v3Start));

	const int32 stepX = v3Dir.x > 0.f ? 1 : (v3Dir.x < 0.f ? -1 : 0);
	const int32 stepZ = v3Dir.z > 0.f ? 1 : (v3Dir.z < 0.f ? -1 : 0);

	constexpr float INF = std::numeric_limits<float>::infinity();
	constexpr float EPS = 1e-6f;

	float tMaxX = INF;
	float tMaxZ = INF;
	float tDeltaX = INF;
	float tDeltaZ = INF;

	const float originX = m_StaticGridDesc.v2OriginXZ.x;
	const float originZ = m_StaticGridDesc.v2OriginXZ.y;

	const float cellSizeX = m_StaticGridDesc.v2CellSizeXZ.x;
	const float cellSizeZ = m_StaticGridDesc.v2CellSizeXZ.y;

	if (stepX != 0 && std::abs(v3Dir.x) > EPS) {
		const float nextBoundaryX =
			originX +
			static_cast<float>(cell.x + (stepX > 0 ? 1 : 0)) * cellSizeX;

		tMaxX = (nextBoundaryX - desc.v3Origin.x) / v3Dir.x;
		tDeltaX = cellSizeX / std::abs(v3Dir.x);
	}

	if (stepZ != 0 && std::abs(v3Dir.z) > EPS) {
		const float nextBoundaryZ =
			originZ +
			static_cast<float>(cell.z + (stepZ > 0 ? 1 : 0)) * cellSizeZ;

		tMaxZ = (nextBoundaryZ - desc.v3Origin.z) / v3Dir.z;
		tDeltaZ = cellSizeZ / std::abs(v3Dir.z);
	}

	float tCurrent = tEnter;

	while (IsValidStaticGridCell(cell.x, cell.z) && tCurrent <= tExit) {
		if (desc.bIncludeStatic) {
			VisitRayCell(
				cell.x,
				cell.z,
				desc,
				xmRayOrigin,
				xmRayDir,
				queryStamp,
				true,
				result
			);
		}

		if (desc.bIncludeDynamic && m_bDynamicGridBuilt) {
			VisitRayCell(
				cell.x,
				cell.z,
				desc,
				xmRayOrigin,
				xmRayDir,
				queryStamp,
				false,
				result
			);
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
			// If ray passes right through the corner
			cell.x += stepX;
			cell.z += stepZ;
			tCurrent = tMaxX;
			tMaxX += tDeltaX;
			tMaxZ += tDeltaZ;
		}
	}

	return result;
}

bool SpatialWorld::IsValidProxyID(SpatialProxyID id) const
{
	return (id != INVALID_SPATIAL_PROXY_ID) && (id < m_Proxies.size());
}

bool SpatialWorld::ShouldTestProxy(const SpatialProxy& proxy, const SpatialQueryDesc& desc) const
{
	if (!proxy.bAlive) {
		return false;
	}

	if (desc.unLayerMask != SPATIAL_NONE) {
		const bool bLayerMatched =
			desc.eLayerMatchMode == SPATIAL_LAYER_MATCH_MODE::ALL
			? HasAllSpatialLayer(proxy.unLayerMask, desc.unLayerMask)
			: HasAnySpatialLayer(proxy.unLayerMask, desc.unLayerMask);

		if (!bLayerMatched) {
			return false;
		}
	}

	if (!desc.bIncludeStatic && HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_STATIC)) {
		return false;
	}

	if (!desc.bIncludeDynamic && HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_DYNAMIC)) {
		return false;
	}

	return true;
}

void SpatialWorld::PushProxyToResult(const SpatialProxy& proxy, SpatialQueryResult& result) const
{
	switch (proxy.type) {
	case SPATIAL_PROXY_TYPE::OBJECT:
		if (proxy.pObject) {
			result.pObjects.push_back(proxy.pObject);
		}
		break;

	case SPATIAL_PROXY_TYPE::TERRAIN:
		if (proxy.pTerrainComponent) {
			result.pTerrainComponents.push_back(proxy.pTerrainComponent);
		}
		break;

	case SPATIAL_PROXY_TYPE::LIGHT:
		if (proxy.pLight) {
			result.pLights.push_back(proxy.pLight);
		}
		break;

	default:
		break;
	}
}

void SpatialWorld::UpdateObjectBounds(SpatialProxy& proxy)
{
	if (!proxy.pObject) {
		proxy.xmAABB = BoundingBox{};
		proxy.xmSphere = BoundingSphere{};
		return;
	}

	const auto pCollider = proxy.pObject->GetComponentFromRoot<ICollider>();
	if (pCollider) {
		proxy.xmAABB = pCollider->GetAABBFromOBBWorld();
		BoundingSphere::CreateFromBoundingBox(proxy.xmSphere, proxy.xmAABB);
		return;
	}

	// Fallback for rendering objects without Collider.
	Vector3 pos = proxy.pObject->GetTransform()->GetPosition();
	proxy.xmAABB.Center = pos;
	proxy.xmAABB.Extents = Vector3{ 1.f, 1.f, 1.f };
	BoundingSphere::CreateFromBoundingBox(proxy.xmSphere, proxy.xmAABB);
}

void SpatialWorld::UpdateTerrainBounds(SpatialProxy& proxy)
{
	if (!proxy.pTerrainComponent) {
		proxy.xmAABB = BoundingBox{};
		proxy.xmSphere = BoundingSphere{};
		return;
	}

	proxy.xmAABB = proxy.pTerrainComponent->GetBounds();
	BoundingSphere::CreateFromBoundingBox(proxy.xmSphere, proxy.xmAABB);
}

void SpatialWorld::UpdateLightBounds(SpatialProxy& proxy)
{
	if (!proxy.pLight) {
		proxy.xmAABB = BoundingBox{};
		proxy.xmSphere = BoundingSphere{};
		return;
	}

	// First, both Point/Spot Light start with spheres.
	// DirectionalLight recommends not to be placed in space segmentation.
	Vector3 pos = proxy.pLight->m_v3Position;
	float range = proxy.pLight->m_fRange;

	proxy.xmSphere.Center = pos;
	proxy.xmSphere.Radius = range;

	BoundingBox::CreateFromSphere(proxy.xmAABB, proxy.xmSphere);
}

uint32 SpatialWorld::BeginSpatialQuery() const
{
	++m_unCurrentQueryStamp;

	if (m_unCurrentQueryStamp == 0) {
		std::fill(m_ProxyQueryStamps.begin(), m_ProxyQueryStamps.end(), 0);
		m_unCurrentQueryStamp = 1;
	}

	return m_unCurrentQueryStamp;
}

bool SpatialWorld::TryMarkProxyVisited(SpatialProxyID proxyID, uint32 queryStamp) const
{
	if (proxyID == INVALID_SPATIAL_PROXY_ID || proxyID >= m_ProxyQueryStamps.size()) {
		return false;
	}

	if (m_ProxyQueryStamps[proxyID] == queryStamp) {
		return false;
	}

	m_ProxyQueryStamps[proxyID] = queryStamp;
	return true;
}

void SpatialWorld::InsertProxyToStaticGrid(SpatialProxyID proxyID)
{
	if (proxyID == INVALID_SPATIAL_PROXY_ID || proxyID >= m_Proxies.size()) {
		return;
	}

	const SpatialProxy& proxy = m_Proxies[proxyID];

	SpatialCellRange range = GetStaticGridCellRangeFromAABB(proxy.xmAABB);

	for (int32 z = range.min.z; z <= range.max.z; ++z) {
		for (int32 x = range.min.x; x <= range.max.x; ++x) {
			if (!IsValidStaticGridCell(x, z)) {
				continue;
			}

			SpatialGridCell& cell = m_StaticGridCells[StaticGridCellToIndex(x, z)];
			cell.proxyIDs.push_back(proxyID);
		}
	}
}

int32 SpatialWorld::StaticGridCellToIndex(int32 x, int32 z) const
{
	return z * static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.x) + x;
}

bool SpatialWorld::IsValidStaticGridCell(int32 x, int32 z) const
{
	return x >= 0 &&
		z >= 0 &&
		x < static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.x) &&
		z < static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.y);
}

SpatialCellCoord SpatialWorld::WorldToStaticGridCellXZ(const Vector3& v3WorldPos) const
{
	const float fx = (v3WorldPos.x - m_StaticGridDesc.v2OriginXZ.x) / m_StaticGridDesc.v2CellSizeXZ.x;
	const float fz = (v3WorldPos.z - m_StaticGridDesc.v2OriginXZ.y) / m_StaticGridDesc.v2CellSizeXZ.y;

	SpatialCellCoord coord{};
	coord.x = static_cast<int32>(std::floor(fx));
	coord.z = static_cast<int32>(std::floor(fz));
	return coord;
}

SpatialCellCoord SpatialWorld::ClampStaticGridCell(SpatialCellCoord coord) const
{
	const int32 maxX = static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.x) - 1;
	const int32 maxZ = static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.y) - 1;

	coord.x = std::clamp(coord.x, 0, maxX);
	coord.z = std::clamp(coord.z, 0, maxZ);
	return coord;
}

SpatialCellRange SpatialWorld::GetStaticGridCellRangeFromAABB(const BoundingBox& xmAABB) const
{
	Vector3 v3Min = xmAABB.Center - xmAABB.Extents;
	Vector3 v3Max = xmAABB.Center + xmAABB.Extents;

	SpatialCellRange range{};
	range.min = ClampStaticGridCell(WorldToStaticGridCellXZ(v3Min));
	range.max = ClampStaticGridCell(WorldToStaticGridCellXZ(v3Max));

	return range;
}

void SpatialWorld::GenerateStaticGridCellBounds()
{
	const int32 cellsX = static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.x);
	const int32 cellsZ = static_cast<int32>(m_StaticGridDesc.xmui2NumCellsXZ.y);

	for (int32 z = 0; z < cellsZ; ++z) {
		for (int32 x = 0; x < cellsX; ++x) {
			const float fMinX = m_StaticGridDesc.v2OriginXZ.x + static_cast<float>(x) * m_StaticGridDesc.v2CellSizeXZ.x;
			const float fMaxX = fMinX + m_StaticGridDesc.v2CellSizeXZ.x;
			const float fMinZ = m_StaticGridDesc.v2OriginXZ.y + static_cast<float>(z) * m_StaticGridDesc.v2CellSizeXZ.y;
			const float fMaxZ = fMinZ + m_StaticGridDesc.v2CellSizeXZ.y;

			const Vector3 v3Min{ fMinX, m_StaticGridDesc.fMinY, fMinZ };

			const Vector3 v3Max{ fMaxX, m_StaticGridDesc.fMaxY, fMaxZ };

			BoundingBox box{};
			BoundingBox::CreateFromPoints(box, v3Min, v3Max);

			m_StaticGridCells[StaticGridCellToIndex(x, z)].xmAABB = box;
		}
	}
}

void SpatialWorld::RebuildDynamicGrid()
{
	if (!m_bStaticGridBuilt) {
		m_bDynamicGridBuilt = false;
		return;
	}

	if (m_DynamicGridCells.size() != m_StaticGridCells.size()) {
		m_DynamicGridCells.resize(m_StaticGridCells.size());
	}

	for (SpatialGridCell& cell : m_DynamicGridCells) {
		cell.proxyIDs.clear();
	}

	for (const SpatialProxy& proxy : m_Proxies) {
		if (!proxy.bAlive) {
			continue;
		}

		if (!proxy.bDynamic) {
			continue;
		}

		InsertProxyToDynamicGrid(proxy.id);
	}

	m_bDynamicGridBuilt = true;
}

void SpatialWorld::InsertProxyToDynamicGrid(SpatialProxyID proxyID)
{
	if (proxyID == INVALID_SPATIAL_PROXY_ID || proxyID >= m_Proxies.size()) {
		return;
	}

	const SpatialProxy& proxy = m_Proxies[proxyID];

	SpatialCellRange range = GetStaticGridCellRangeFromAABB(proxy.xmAABB);

	for (int32 z = range.min.z; z <= range.max.z; ++z) {
		for (int32 x = range.min.x; x <= range.max.x; ++x) {
			if (!IsValidStaticGridCell(x, z)) {
				continue;
			}

			SpatialGridCell& cell =
				m_DynamicGridCells[StaticGridCellToIndex(x, z)];

			cell.proxyIDs.push_back(proxyID);
		}
	}
}

bool SpatialWorld::IntersectRayGridXZ(const Vector3& v3Origin, const Vector3& v3Dir, float fMaxDistance, OUT float& outTEnter, OUT float& outTExit) const
{
	const float minX = m_StaticGridDesc.v2OriginXZ.x;
	const float minZ = m_StaticGridDesc.v2OriginXZ.y;

	const float maxX =
		minX +
		m_StaticGridDesc.v2CellSizeXZ.x *
		static_cast<float>(m_StaticGridDesc.xmui2NumCellsXZ.x);

	const float maxZ =
		minZ +
		m_StaticGridDesc.v2CellSizeXZ.y *
		static_cast<float>(m_StaticGridDesc.xmui2NumCellsXZ.y);

	float tEnter = 0.f;
	float tExit = fMaxDistance;

	constexpr float EPS = 1e-6f;

	auto updateSlab = [&](float origin, float dir, float slabMin, float slabMax) -> bool
		{
			if (std::abs(dir) < EPS) {
				return origin >= slabMin && origin <= slabMax;
			}

			float t1 = (slabMin - origin) / dir;
			float t2 = (slabMax - origin) / dir;

			if (t1 > t2) {
				std::swap(t1, t2);
			}

			tEnter = std::max(tEnter, t1);
			tExit = std::min(tExit, t2);

			return tEnter <= tExit;
		};

	if (!updateSlab(v3Origin.x, v3Dir.x, minX, maxX)) {
		return false;
	}

	if (!updateSlab(v3Origin.z, v3Dir.z, minZ, maxZ)) {
		return false;
	}

	if (tExit < 0.f) {
		return false;
	}

	tEnter = std::max(0.f, tEnter);
	tExit = std::min(fMaxDistance, tExit);

	if (tEnter > tExit) {
		return false;
	}

	outTEnter = tEnter;
	outTExit = tExit;
	return true;
}

void SpatialWorld::VisitRayCell(int32 x, int32 z, const SpatialRayQueryDesc& desc, const XMVECTOR& xmRayOrigin, const XMVECTOR& xmRayDir, uint32 queryStamp, bool bStaticCell, SpatialRayQueryResult& result) const
{
	if (!IsValidStaticGridCell(x, z)) {
		return;
	}

	const std::vector<SpatialGridCell>& cells =
		bStaticCell ? m_StaticGridCells : m_DynamicGridCells;

	if (cells.empty()) {
		return;
	}

	const SpatialGridCell& cell =
		cells[StaticGridCellToIndex(x, z)];

	for (SpatialProxyID proxyID : cell.proxyIDs) {
		if (!TryMarkProxyVisited(proxyID, queryStamp)) {
			continue;
		}

		if (proxyID >= m_Proxies.size()) {
			continue;
		}

		const SpatialProxy& proxy = m_Proxies[proxyID];

		if (!ShouldTestProxyRay(proxy, desc)) {
			continue;
		}

		// Ray query only allows object candidates
		if (proxy.type != SPATIAL_PROXY_TYPE::OBJECT) {
			continue;
		}

		BoundingBox proxyAABB = proxy.xmAABB;
		proxyAABB.Extents.x += desc.fAABBInflation;
		proxyAABB.Extents.y += desc.fAABBInflation;
		proxyAABB.Extents.z += desc.fAABBInflation;

		float fDist = 0.f;
		if (!proxyAABB.Intersects(xmRayOrigin, xmRayDir, fDist)) {
			continue;
		}

		if (fDist < 0.f || fDist > desc.fMaxDistance) {
			continue;
		}

		PushProxyToRayResult(proxy, result);
	}
}

bool SpatialWorld::ShouldTestProxyRay(const SpatialProxy& proxy, const SpatialRayQueryDesc& desc) const
{
	if (!proxy.bAlive) {
		return false;
	}

	if (desc.unLayerMask != SPATIAL_NONE) {
		const bool bLayerMatched =
			desc.eLayerMatchMode == SPATIAL_LAYER_MATCH_MODE::ALL
			? HasAllSpatialLayer(proxy.unLayerMask, desc.unLayerMask)
			: HasAnySpatialLayer(proxy.unLayerMask, desc.unLayerMask);

		if (!bLayerMatched) {
			return false;
		}
	}

	if (!desc.bIncludeStatic && HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_STATIC)) {
		return false;
	}

	if (!desc.bIncludeDynamic && HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_DYNAMIC)) {
		return false;
	}

	return true;
}

void SpatialWorld::PushProxyToRayResult(const SpatialProxy& proxy, SpatialRayQueryResult& result) const
{
	if (proxy.type != SPATIAL_PROXY_TYPE::OBJECT) {
		return;
	}

	if (!proxy.pObject) {
		return;
	}

	SpatialObjectQueryItem item{};
	item.pObject = proxy.pObject;
	item.proxyID = proxy.id;
	item.objectTypeID = proxy.objectTypeID;

	result.objects.push_back(item);
}

DirectX::BoundingBox SpatialWorld::MakeAABBFromFrustum(const BoundingFrustum& xmFrustumWorld) const
{
	std::array<Vector3, BoundingFrustum::CORNER_COUNT> corners{};
	xmFrustumWorld.GetCorners(corners.data());

	BoundingBox box{};
	BoundingBox::CreateFromPoints(
		box,
		static_cast<size_t>(corners.size()),
		corners.data(),
		sizeof(Vector3)
	);

	return box;
}

void SpatialWorld::DebugPrintProxyStats() const
{
	int32 nAlive = 0;
	int32 nRenderable = 0;
	int32 nStaticCount = 0;
	int32 nDynamicCount = 0;
	int32 nDynamicRenderable = 0;

	for (const SpatialProxy& proxy : m_Proxies) {
		if (!proxy.bAlive) {
			continue;
		}

		++nAlive;

		if (HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_RENDERABLE)) {
			++nRenderable;
		}

		if (HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_STATIC)) {
			++nStaticCount;
		}

		if (HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_DYNAMIC)) {
			++nDynamicCount;

			if (HasAnySpatialLayer(proxy.unLayerMask, SPATIAL_RENDERABLE)) {
				++nDynamicRenderable;
			}
		}
	}

	ImGui::Text("Spatial Proxies: %d", (int)m_Proxies.size());
	ImGui::Text("Alive: %d", nAlive);
	ImGui::Text("Renderable: %d", nRenderable);
	ImGui::Text("Static: %d", nStaticCount);
	ImGui::Text("Dynamic: %d", nDynamicCount);
	ImGui::Text("Dynamic Renderable: %d", nDynamicRenderable);
	ImGui::Text("QueryStamps: %d", (int)m_ProxyQueryStamps.size());
}
