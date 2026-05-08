#pragma once
#include "SpatialTypes.h"
#include "GameObject.h"
#include "Collider.h"
#include "TerrainObject.h"
#include "Light.h"

class SpatialWorld
{
public:
	void Clear();

public:
	template<typename T>
	SpatialProxyID RegisterObject(
		T* pObject,
		uint32 unLayerMask,
		bool bDynamic);

	SpatialProxyID RegisterTerrain(
		TerrainComponent* pTerrainComponent,
		uint32 unLayerMask);

	SpatialProxyID RegisterLight(
		Light* pLight,
		uint32 unLayerMask,
		bool bDynamic);

public:
	void UnregisterObject(IGameObject* pObject);
	void UnregisterTerrain(TerrainComponent* pTerrainComponent);
	void UnregisterLight(Light* pLight);

public:
	void BuildStaticGrid(const SpatialGridBuildDesc& desc);

public:
	void MarkObjectDirty(IGameObject* pObject);
	void UpdateDirtyProxies();

public:
	SpatialQueryResult QueryFrustumLinear(
		const BoundingFrustum& xmFrustumWorld,
		const SpatialQueryDesc& desc) const;

	SpatialQueryResult QueryFrustum(
		const BoundingFrustum& xmFrustumWorld,
		const SpatialQueryDesc& desc) const;

	SpatialQueryResult QueryAABB(
		const BoundingBox& xmAABB,
		const SpatialQueryDesc& desc) const;

	SpatialQueryResult QuerySphere(
		const BoundingSphere& xmSphere,
		const SpatialQueryDesc& desc) const;

public:
	SpatialRayQueryResult QueryRay(const SpatialRayQueryDesc& desc) const;

public:
	void DebugPrintProxyStats() const;

private:
	bool IsValidProxyID(SpatialProxyID id) const;
	bool ShouldTestProxy(const SpatialProxy& proxy, const SpatialQueryDesc& desc) const;
	void PushProxyToResult(const SpatialProxy& proxy, SpatialQueryResult& result) const;

private:
	SpatialProxyID RegisterObjectImpl(
		IGameObject* pObject, 
		uint32 unLayerMask, 
		bool bDynamic, 
		std::type_index objectType);


	void UpdateObjectBounds(SpatialProxy& proxy);
	void UpdateTerrainBounds(SpatialProxy& proxy);
	void UpdateLightBounds(SpatialProxy& proxy);

private:
	uint32 BeginSpatialQuery() const;
	bool TryMarkProxyVisited(SpatialProxyID proxyID, uint32 queryStamp) const;

private:
	void InsertProxyToStaticGrid(SpatialProxyID proxyID);
	int32 StaticGridCellToIndex(int32 x, int32 z) const;
	bool IsValidStaticGridCell(int32 x, int32 z) const;
	SpatialCellCoord WorldToStaticGridCellXZ(const Vector3& v3WorldPos) const;
	SpatialCellCoord ClampStaticGridCell(SpatialCellCoord coord) const;
	SpatialCellRange GetStaticGridCellRangeFromAABB(const BoundingBox& xmAABB) const;
	void GenerateStaticGridCellBounds();

private:
	void RebuildDynamicGrid();
	void InsertProxyToDynamicGrid(SpatialProxyID proxyID);

	bool IntersectRayGridXZ(
		const Vector3& v3Origin,
		const Vector3& v3Dir,
		float fMaxDistance,
		OUT float& outTEnter,
		OUT float& outTExit) const;

	void VisitRayCell(
		int32 x,
		int32 z,
		const SpatialRayQueryDesc& desc,
		const XMVECTOR& xmRayOrigin,
		const XMVECTOR& xmRayDir,
		uint32 queryStamp,
		bool bStaticCell,
		SpatialRayQueryResult& result) const;

	bool ShouldTestProxyRay(
		const SpatialProxy& proxy,
		const SpatialRayQueryDesc& desc) const;

	void PushProxyToRayResult(
		const SpatialProxy& proxy,
		SpatialRayQueryResult& result) const;

private:
	BoundingBox MakeAABBFromFrustum(const BoundingFrustum& xmFrustumWorld) const;

private:
	SpatialGridBuildDesc m_StaticGridDesc{};
	std::vector<SpatialGridCell> m_StaticGridCells;
	bool m_bStaticGridBuilt = false;
	
	std::vector<SpatialGridCell> m_DynamicGridCells;
	bool m_bDynamicGridBuilt = false;

	mutable uint32 m_unCurrentQueryStamp = 0;
	mutable std::vector<uint32> m_ProxyQueryStamps;

private:
	std::vector<SpatialProxy> m_Proxies;

	std::unordered_map<IGameObject*, SpatialProxyID> m_ObjectToProxy;
	std::unordered_map<TerrainComponent*, SpatialProxyID> m_TerrainToProxy;
	std::unordered_map<Light*, SpatialProxyID> m_LightToProxy;

};
template<typename T>
SpatialProxyID SpatialWorld::RegisterObject(T* pObject, uint32 unLayerMask, bool bDynamic)
{
	static_assert(std::derived_from<T, IGameObject>);

	return RegisterObjectImpl(
		pObject,
		unLayerMask,
		bDynamic,
		std::type_index(typeid(T))
	);
}
