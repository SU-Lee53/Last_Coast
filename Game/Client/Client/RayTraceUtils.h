#pragma once
//#include "Zombie.h"
//#include "StaticObject.h"

enum class TRACE_HIT_TYPE : uint8
{
	NONE,
	ZOMBIE,
	STATIC_OBJECT,
	WEAPON_OBJECT,
};

struct RayTraceDesc
{
	Vector3 v3Origin = Vector3::Zero;
	Vector3 v3Direction = Vector3::Backward;
	float fMaxDistance = 5000.f;

	float fDamage = 0.f;

	IGameObject* pInstigator = nullptr;
	IGameObject* pSourceObject = nullptr;
};

struct RayTraceHitResult
{
	bool bBlockingHit = false;

	TRACE_HIT_TYPE eHitType = TRACE_HIT_TYPE::NONE;

	float fDistance = std::numeric_limits<float>::max();

	Vector3 v3Origin = Vector3::Zero;
	Vector3 v3Direction = Vector3::Zero;
	Vector3 v3ImpactPoint = Vector3::Zero;
	Vector3 v3ImpactNormal = Vector3::Zero;

	float fDamage = 0.f;

	IGameObject* pInstigator = nullptr;
	IGameObject* pSourceObject = nullptr;
	IGameObject* pHitObject = nullptr;
};

////////////////////////////////////////////////////////////
// Trace test per object types
template<typename T>
struct TraceHitTester
{
	static bool Intersects(const std::shared_ptr<T>& pObj, const Vector3& rayOrigin, const Vector3& rayDir, OUT float& outDist)
	{
		static_assert(sizeof(T) == 0, "TraceHitTester<T> specialization required.");
		return false;
	}
	static bool Intersects(T* pObj, const Vector3& rayOrigin, const Vector3& rayDir, OUT float& outDist)
	{
		static_assert(sizeof(T) == 0, "TraceHitTester<T> specialization required.");
		return false;
	}
};
