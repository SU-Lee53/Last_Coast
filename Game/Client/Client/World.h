#pragma once
#include "TypedObjectPool.h"
#include "RayTraceUtils.h"

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
		GetObjects<T>().Remove(pObj);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	bool Contains(const std::shared_ptr<T>& pObj) const {
		return GetObjects<T>().Contains(pObj);
	}

	template<typename T> requires OneOf<T, ObjTypes...>
	void Clear() {
		std::get<TypedObjectPool<T>>(m_Objects).Clear();
	}

	void ClearAll() {
		std::apply(
			[](auto&... pools) {
				(pools.Clear(), ...);
			},
			m_Objects
		);
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
		return GetObjects<T>().RemoveIfAlive(std::forward<Func>(func), std::forward<Args>(args)...);
	}

public:
	// Ray(line)trace
	template<typename... TargetTypes> requires (OneOf<TargetTypes, ObjTypes...> && ...)
	bool LineTraceSingle(const RayTraceDesc& desc, OUT RayTraceHitResult& outHit) const;

private:
	// Ray(line) trace helper
	template<typename T>
	void CheckTraceAgainstPool(
		const RayTraceDesc& desc,
		const Vector3& v3Dir,
		const XMVECTOR& rayOrigin,
		const XMVECTOR& rayDir,
		OUT float& fBestDist,
		OUT bool& bHit,
		OUT RayTraceHitResult& outHit) const;

private:
	std::tuple<TypedObjectPool<ObjTypes>...> m_Objects;
};



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

	bool bHit = false;
	float fBestDist = desc.fMaxDistance;

	(CheckTraceAgainstPool<TargetTypes>(
		desc,
		v3Dir,
		rayOrigin,
		rayDir,
		fBestDist,
		bHit,
		outHit), ...);

	if (bHit && outHit.pHitObject) {
		outHit.pHitObject->OnTraceHit(outHit);
	}

	return bHit;
}

template<typename... ObjTypes> requires (std::derived_from<ObjTypes, IGameObject> && ...)
template<typename T>
void World<ObjTypes...>::CheckTraceAgainstPool(const RayTraceDesc& desc, const Vector3& v3Dir, const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, OUT float& fBestDist, OUT bool& bHit, OUT RayTraceHitResult& outHit) const
{
	for (const auto& pObj : GetObjects<T>()) {
		if (!pObj) {
			continue;
		}

		if (pObj == desc.pInstigator || pObj == desc.pSourceObject) {
			continue;
		}

		float fDist = 0.f;
		if (!TraceHitTester<T>::Intersects(pObj, rayOrigin, rayDir, fDist)) {
			continue;
		}

		if (fDist < 0.f || fDist > fBestDist) {
			continue;
		}

		fBestDist = fDist;
		bHit = true;

		outHit.bBlockingHit = true;
		outHit.eHitType = TraceHitTester<T>::HitType;
		outHit.fDistance = fDist;

		outHit.v3Origin = desc.v3Origin;
		outHit.v3Direction = v3Dir;
		outHit.v3ImpactPoint = desc.v3Origin + v3Dir * fDist;
		outHit.v3ImpactNormal = -v3Dir; // 임시

		outHit.fDamage = desc.fDamage;
		outHit.pInstigator = desc.pInstigator;
		outHit.pSourceObject = desc.pSourceObject;
		outHit.pHitObject = pObj;
	}
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
