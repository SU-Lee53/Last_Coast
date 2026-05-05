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
	static bool Intersects(
		const std::shared_ptr<StaticObject>& pObj,
		const XMVECTOR& rayOrigin,
		const XMVECTOR& rayDir,
		OUT float& outDist)
	{
		auto pCollider = pObj->GetComponent<StaticCollider>();
		if (!pCollider) {
			return false;
		}

		return pCollider->GetOBBWorld().Intersects(rayOrigin, rayDir, outDist);
	}

	static constexpr TRACE_HIT_TYPE HitType = TRACE_HIT_TYPE::STATIC_OBJECT;
};
