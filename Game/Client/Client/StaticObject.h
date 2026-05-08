#pragma once
#include "GameObject.h"
class StaticObject : public IGameObject {
public:
	StaticObject() : IGameObject{ OBJECT_MOBILITY_TYPE::STATIC } {}

	virtual void Initialize() override;
	virtual void ProcessInput() override;
	virtual void PreUpdate() override;
	virtual void Update() override;
	virtual void PostUpdate() override;

	virtual void OnTraceHit(const RayTraceHitResult& hitResult) override;

};

template<>
struct TraceHitTester<StaticObject>
{
	static bool Intersects(StaticObject* pObj, Vector3 rayOrigin, Vector3 rayDir, OUT float& outDist)
	{
		if (!pObj) {
			return false;
		}

		auto pCollider = pObj->GetComponent<StaticCollider>();
		if (!pCollider) {
			return false;
		}

		return pCollider->GetOBBWorld().Intersects(rayOrigin, rayDir, outDist);
	}

	static bool Intersects(const std::shared_ptr<StaticObject>& pObj, Vector3 rayOrigin, Vector3 rayDir, OUT float& outDist)
	{
		return Intersects(pObj.get(), rayOrigin, rayDir, outDist);
	}

	static constexpr TRACE_HIT_TYPE HitType = TRACE_HIT_TYPE::STATIC_OBJECT;
};
