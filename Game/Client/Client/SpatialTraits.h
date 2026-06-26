#pragma once
#include "SpatialTypes.h"
#include "Zombie.h"
#include "StaticObject.h"
#include "WeaponObject.h"
#include "ThirdPersonPlayer.h"
#include "CrashDebris.h"

template<typename T>
struct SpatialObjectTraits
{
	static constexpr bool bSpatial = false;
	static constexpr uint32 unLayerMask = SPATIAL_NONE;
	static constexpr bool bDynamic = false;
};
template<>
struct SpatialObjectTraits<StaticObject>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_COLLIDABLE |
        SPATIAL_RAY_TARGET |
        SPATIAL_CAST_SHADOW;

    static constexpr bool bDynamic = false;
};

template<>
struct SpatialObjectTraits<Zombie>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_COLLIDABLE |
        SPATIAL_RAY_TARGET |
        SPATIAL_CAST_SHADOW;

    static constexpr bool bDynamic = true;
};

template<>
struct SpatialObjectTraits<WeaponObject>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_CAST_SHADOW;

    static constexpr bool bDynamic = true;
};

template<>
struct SpatialObjectTraits<NetworkRemoteThirdPersonPlayer>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_CAST_SHADOW;

    static constexpr bool bDynamic = true;
};

template<>
struct SpatialObjectTraits<CrashDebris>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_CAST_SHADOW;

    // 잔해는 움직이지 않지만, 정적 그리드는 PostInitialize에서 고정되므로
    // 런타임에 추가하려면 dynamic 으로 등록해야 한다(좀비와 동일 경로).
    static constexpr bool bDynamic = true;
};
