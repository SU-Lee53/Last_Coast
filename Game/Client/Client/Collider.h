#pragma once
#include "Component.h"

class Collider : public IComponent {
public:
	Collider(std::shared_ptr<GameObject> pOwner);

	virtual void Initialize() override;

	bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld);

private:
	void MergeOBB(std::shared_ptr<GameObject> pObj);

protected:
	BoundingOrientedBox m_xmOBBOrigin;
	BoundingOrientedBox m_xmOBBWorld;

};

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

class StaticCollider : public Collider {
public:
	StaticCollider(std::shared_ptr<GameObject> pOwner);
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<GameObject> pNewOwner) override;
};

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

class DynamicCollider : public Collider {
public:
	DynamicCollider(std::shared_ptr<GameObject> pOwner);
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<GameObject> pNewOwner) override;
};

template <>
struct ComponentIndex<Collider> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::COLLIDER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::COLLIDER);
};

template <>
struct ComponentIndex<StaticCollider> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::COLLIDER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::COLLIDER);
};

template <>
struct ComponentIndex<DynamicCollider> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::COLLIDER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::COLLIDER);
};
