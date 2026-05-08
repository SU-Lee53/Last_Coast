#pragma once

class IGameObject;
class TerrainComponent;
class Light;

// mask 사용을 위해 enum class 가 아닌 enum으로 선언
// 공간 안의 Proxy 들의 기능을 정의해는 Tag
enum SPATIAL_LAYER : uint32 {
	SPATIAL_NONE = 0,
	SPATIAL_RENDERABLE = 1,
	SPATIAL_COLLIDABLE = SPATIAL_RENDERABLE << 1,
	SPATIAL_RAY_TARGET = SPATIAL_COLLIDABLE << 1,
	SPATIAL_TERRAIN = SPATIAL_RAY_TARGET << 1,
	SPATIAL_LIGHT = SPATIAL_TERRAIN << 1,
	SPATIAL_CAST_SHADOW = SPATIAL_LIGHT << 1,
	SPATIAL_STATIC = SPATIAL_CAST_SHADOW << 1,
	SPATIAL_DYNAMIC = SPATIAL_STATIC << 1
};

enum class SPATIAL_LAYER_MATCH_MODE : uint8
{
	ANY,
	ALL,
};

inline bool HasAnySpatialLayer(uint32 unMask, uint32 unQueryMask)
{
	return (unMask & unQueryMask) != 0;
}

inline bool HasAllSpatialLayer(uint32 unMask, uint32 unQueryMask)
{
	if (unQueryMask == SPATIAL_NONE) {
		return true;
	}

	return (unMask & unQueryMask) == unQueryMask;
}

using SpatialProxyID = uint32;
constexpr SpatialProxyID INVALID_SPATIAL_PROXY_ID = std::numeric_limits<uint32>::max();
constexpr uint32 INVALID_SPATIAL_OBJECT_TYPE_ID = std::numeric_limits<uint32>::max();

enum class SPATIAL_PROXY_TYPE : uint8
{
	NONE,
	OBJECT,
	TERRAIN,
	LIGHT,
};

struct SpatialCellCoord
{
	int32 x = 0;
	int32 z = 0;
};

struct SpatialCellRange
{
	SpatialCellCoord min;
	SpatialCellCoord max;
};

struct SpatialGridCell
{
	BoundingBox xmAABB{};
	std::vector<SpatialProxyID> proxyIDs;
};

struct SpatialGridBuildDesc
{
	Vector2 v2OriginXZ{};
	Vector2 v2CellSizeXZ{ 100.f, 100.f };
	XMUINT2 xmui2NumCellsXZ{ 1, 1 };

	float fMinY = -1000.f;
	float fMaxY = +1000.f;
};

struct SpatialProxy
{
	SpatialProxyID id = INVALID_SPATIAL_PROXY_ID;

	SPATIAL_PROXY_TYPE type = SPATIAL_PROXY_TYPE::NONE;

	IGameObject* pObject = nullptr;
	TerrainComponent* pTerrainComponent = nullptr;
	Light* pLight = nullptr;

	//uint32 unObjectTypeID = INVALID_SPATIAL_OBJECT_TYPE_ID;
	std::type_index objectTypeID{ typeid(void) };

	BoundingBox xmAABB{};
	BoundingSphere xmSphere{};

	uint32 unLayerMask = SPATIAL_NONE;

	bool bDynamic = false;
	bool bAlive = true;
	bool bDirty = true;
};

struct SpatialQueryDesc
{
	uint32 unLayerMask = SPATIAL_NONE;
	SPATIAL_LAYER_MATCH_MODE eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;

	bool bIncludeStatic = true;
	bool bIncludeDynamic = true;

	Vector3 v3AABBInflation = Vector3::Zero;
};

struct SpatialQueryResult
{
	std::vector<IGameObject*> pObjects;
	std::vector<TerrainComponent*> pTerrainComponents;
	std::vector<Light*> pLights;

	void Clear()
	{
		pObjects.clear();
		pTerrainComponents.clear();
		pLights.clear();
	}

	void Reserve(size_t n)
	{
		pObjects.reserve(n);
		pTerrainComponents.reserve(n);
		pLights.reserve(n);
	}
};

struct SpatialRayQueryDesc
{
	Vector3 v3Origin = Vector3::Zero;
	Vector3 v3Direction = Vector3::Backward;
	float fMaxDistance = 0.f;

	uint32 unLayerMask = SPATIAL_RAY_TARGET;
	SPATIAL_LAYER_MATCH_MODE eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;

	bool bIncludeStatic = true;
	bool bIncludeDynamic = true;

	float fAABBInflation = 1.f;
};

struct SpatialObjectQueryItem
{
	IGameObject* pObject = nullptr;
	SpatialProxyID proxyID = INVALID_SPATIAL_PROXY_ID;
	std::type_index objectTypeID{ typeid(void) };
};

struct SpatialRayQueryResult
{
	std::vector<SpatialObjectQueryItem> objects;

	void Clear()
	{
		objects.clear();
	}

	void Reserve(size_t n)
	{
		objects.reserve(n);
	}
};
