#pragma once
#include "SpatialTypes.h"
#include "Zombie.h"
#include "StaticObject.h"
#include "WaterGridObject.h"
#include "WeaponObject.h"
#include "ThirdPersonPlayer.h"

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
struct SpatialObjectTraits<WaterGridObject>
{
    static constexpr bool bSpatial = true;
    static constexpr uint32 unLayerMask =
        SPATIAL_RENDERABLE |
        SPATIAL_RAY_TARGET;

    static constexpr bool bDynamic = true;
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
