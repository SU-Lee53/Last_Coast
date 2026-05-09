#pragma once
#include "TypedObjectPool.h"
#include "RayTraceUtils.h"
#include "SpatialWorld.h"
#include "SpatialTraits.h"

template<typename T, typename... Types>
concept OneOf = (std::same_as<T, Types> || ...);

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
class World {
public:
	template<typename T> requires OneOf<T, ObjTypes...>
	TypedObjectPool<T>& GetObjects() {
		return std::get<TypedObjectPool<T>>(m_Objects);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	const TypedObjectPool<T>& GetObjects() const {
		return std::get<TypedObjectPool<T>>(m_Objects);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void Reserve(size_t nNewSize) {
		std::get<TypedObjectPool<T>>(m_Objects).Reserve(nNewSize);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void Add(const std::shared_ptr<T>& pObj) {
		GetObjects<T>().Add(pObj);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void Remove(const std::shared_ptr<T>& pObj) {
		if (!pObj) {
			return;
		}

		m_SpatialWorld.UnregisterObject(pObj.get());
		GetObjects<T>().Remove(pObj);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	bool Contains(const std::shared_ptr<T>& pObj) const {
		return GetObjects<T>().Contains(pObj);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void Clear() {
		ForEachAlive<T>([this](const std::shared_ptr<T>& pObj) {
			if (pObj) {
				m_SpatialWorld.UnregisterObject(pObj.get());
			}
		});

		std::get<TypedObjectPool<T>>(m_Objects).Clear();
	}

	void ClearAll() {
		std::apply(
			[](auto&... pools) {
				(pools.Clear(), ...);
			},
			m_Objects
		);

		m_SpatialWorld.Clear();
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	size_t Size() const {
		return std::get<TypedObjectPool<T>>(m_Objects).Size();
	}

	size_t SizeAll() const {
		size_t ret = 0;
		std::apply(
			[&ret](const auto&... pools) {
				((ret += pools.Size()), ...);
			},
			m_Objects
		);

		return ret;
	}

public:
	// Life-Cycle
	void IntiializeObjects();
	void PreProcessInput();
	void PostProcessInput();
	void PreUpdate();
	void FixedUpdate();
	void PostUpdate();
	void PrepareRender();


	template<typename T> requires OneOf<T, ObjTypes...>
	void PrepareRender() {
		ForEachAlive<T>([](auto& obj) {
			obj->Render();
		});
	}

public:
	// 순회하며 Callback 적용
	// 1. 전체에서 :
	//	1-1. Callback + 인자 전달
	template<typename Func, typename... Args>
	void ForEachAliveAll(Func&& func, Args&&... args) {
		std::apply(
			[&](auto&... pools) {
				(pools.ForEachAlive(func, args...), ...);
			},
			m_Objects
		);
	}

	template<typename Func, typename... Args>
	void ForEachAliveAll(Func&& func, Args&&... args) const {
		std::apply(
			[&](const auto&... pools) {
				(pools.ForEachAlive(func, args...), ...);
			},
			m_Objects
		);
	}
	
	// 2. 일부에서 :
	//	2-1. Callback + 인자 전달
	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	void ForEachAlive(Func&& func, Args&&... args) {
		GetObjects<T>().ForEachAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	void ForEachAlive(Func&& func, Args&&... args) const {
		GetObjects<T>().ForEachAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	//	2-2. Callback + 결과 수집
	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto CollectAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().CollectAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	//	2-3. 조건 검사
	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto AllOfAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().AllOfAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto AnyOfAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().AnyOfAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	//	2-4. 객체 탐색
	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto FindIfAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().FindIfAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto FindAllIfAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().FindAllIfAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template<typename T, typename Func, typename... Args> requires OneOf<T, ObjTypes...>
	auto RemoveIfAlive(Func&& func, Args&&... args) {
		return GetObjects<T>().RemoveIfAlive(
			[this, &func, &args...](const std::shared_ptr<T>& pObj) -> bool
			{
				if (!pObj) {
					return false;
				}

				const bool bRemove = std::invoke(
					func,
					std::static_pointer_cast<IGameObject>(pObj),
					std::forward<Args>(args)...
				);

				if (bRemove) {
					m_SpatialWorld.UnregisterObject(pObj.get());
				}

				return bRemove;
			}
		);
	}

public:
	// Ray(line)trace
	template<typename... TargetTypes> requires (OneOf<TargetTypes, ObjTypes...> && ...)
	bool LineTraceSingle(const RayTraceDesc& desc, OUT RayTraceHitResult& outHit) const;

public:
	SpatialWorld& GetSpatial()
	{
		return m_SpatialWorld;
	}

	const SpatialWorld& GetSpatial() const
	{
		return m_SpatialWorld;
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void RegisterSpatialObject(
		const std::shared_ptr<T>& pObj,
		uint32 unLayerMask,
		bool bDynamic);

	template<typename T> requires OneOf<T, ObjTypes...>
	void UnregisterSpatialObject(const std::shared_ptr<T>& pObj);

	void UpdateSpatial()
	{
		m_SpatialWorld.UpdateDirtyProxies();
	}

	template<bool bDynamicTarget>
	void RegisterSpatialObjectsByMobility()
	{
		(RegisterSpatialObjectsByMobilityImpl<ObjTypes, bDynamicTarget>(), ...);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void RegisterSpatialObject(const std::shared_ptr<T>& pObj)
	{
		if (!pObj) {
			return;
		}

		if constexpr (SpatialObjectTraits<T>::bSpatial) {
			m_SpatialWorld.RegisterObject<T>(
				pObj.get(),
				SpatialObjectTraits<T>::unLayerMask,
				SpatialObjectTraits<T>::bDynamic
			);
		}
	}

private:

	template<typename T> requires OneOf<T, ObjTypes...>
	void RegisterSpatialObjects()
	{
		if constexpr (SpatialObjectTraits<T>::bSpatial) {
			for (auto& pObj : GetObjects<T>()) {
				RegisterSpatialObject<T>(pObj);
			}
		}
	}

	template<typename T, bool bDynamicTarget>
	void RegisterSpatialObjectsByMobilityImpl()
	{
		if constexpr (OneOf<T, ObjTypes...> && SpatialObjectTraits<T>::bSpatial) {
			if constexpr (SpatialObjectTraits<T>::bDynamic == bDynamicTarget) {
				RegisterSpatialObjects<T>();
			}
		}
	}

private:
	std::tuple<TypedObjectPool<ObjTypes>...> m_Objects;
	SpatialWorld m_SpatialWorld;
};

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
template<typename T> requires OneOf<T, ObjTypes...>
void World<ObjTypes...>::RegisterSpatialObject(const std::shared_ptr<T>& pObj, uint32 unLayerMask, bool bDynamic)
{
	if (!pObj) {
		return;
	}

	m_SpatialWorld.RegisterObject<T>(pObj.get(), unLayerMask, bDynamic);
}

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
template<typename T> requires OneOf<T, ObjTypes...>
void World<ObjTypes...>::UnregisterSpatialObject(const std::shared_ptr<T>& pObj)
{
	if (!pObj) {
		return;
	}

	m_SpatialWorld.UnregisterObject(pObj.get());
}

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
template<typename... TargetTypes> requires (OneOf<TargetTypes, ObjTypes...> && ...)
bool World<ObjTypes...>::LineTraceSingle(const RayTraceDesc& desc, OUT RayTraceHitResult& outHit) const
{
	outHit = {};

	Vector3 v3Dir = desc.v3Direction;
	if (v3Dir.LengthSquared() <= 1e-8f) {
		return false;
	}

	v3Dir.Normalize();

	const XMVECTOR rayOrigin = XMLoadFloat3(&desc.v3Origin);
	const XMVECTOR rayDir = XMLoadFloat3(&v3Dir);

	SpatialRayQueryDesc rayQuery{};
	rayQuery.v3Origin = desc.v3Origin;
	rayQuery.v3Direction = v3Dir;
	rayQuery.fMaxDistance = desc.fMaxDistance;
	rayQuery.unLayerMask = SPATIAL_RAY_TARGET;
	rayQuery.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
	rayQuery.bIncludeStatic = true;
	rayQuery.bIncludeDynamic = true;
	rayQuery.fAABBInflation = 2.f;

	SpatialRayQueryResult candidates =
		m_SpatialWorld.QueryRay(rayQuery);

	bool bHit = false;
	float fBestDist = desc.fMaxDistance;

	auto testCandidate = [&]<typename T>(const SpatialObjectQueryItem & candidate)
	{
		static const std::type_index targetType{ typeid(T) };

		if (candidate.objectTypeID != targetType) {
			return;
		}

		IGameObject* pCandidate = candidate.pObject;
		if (!pCandidate) {
			return;
		}

		if (pCandidate == desc.pInstigator ||
			pCandidate == desc.pSourceObject) {
			return;
		}

		T* pTyped = static_cast<T*>(pCandidate);

		float fDist = 0.f;
		if (!TraceHitTester<T>::Intersects(pTyped, rayOrigin, rayDir, fDist)) {
			return;
		}

		if (fDist < 0.f || fDist > fBestDist) {
			return;
		}

		fBestDist = fDist;
		bHit = true;

		outHit.bBlockingHit = true;
		outHit.eHitType = TraceHitTester<T>::HitType;
		outHit.fDistance = fDist;

		outHit.v3Origin = desc.v3Origin;
		outHit.v3Direction = v3Dir;
		outHit.v3ImpactPoint = desc.v3Origin + v3Dir * fDist;
		outHit.v3ImpactNormal = -v3Dir; // Temporary

		outHit.fDamage = desc.fDamage;
		outHit.pInstigator = desc.pInstigator;
		outHit.pSourceObject = desc.pSourceObject;
		outHit.pHitObject = pCandidate;
	};

	for (const SpatialObjectQueryItem& candidate : candidates.objects) {
		(testCandidate.template operator() < TargetTypes > (candidate), ...);
	}

	if (bHit && outHit.pHitObject) {
		outHit.pHitObject->OnTraceHit(outHit);
	}

	return bHit;
}

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::IntiializeObjects()
{
	ForEachAliveAll([](auto& obj) {
		obj->Initialize();
	});
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::PreProcessInput()
{
	
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::PostProcessInput()
{
	ForEachAliveAll([](auto& obj) {
		obj->ProcessInput();
	});
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::PreUpdate()
{
	ForEachAliveAll([](auto& obj) {
		obj->PreUpdate();
	});
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::FixedUpdate()
{
	ForEachAliveAll([](auto& obj) {
		obj->Update();
	});
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::PostUpdate()
{
	ForEachAliveAll([](auto& obj) {
		obj->PostUpdate();
	});
}

template<typename... ObjTypes>
	requires (std::derived_from<ObjTypes, IGameObject> && ...)
void World<ObjTypes...>::PrepareRender()
{
	ForEachAliveAll([](auto& obj) {
		obj->Render();
	});
}
