#pragma once
#include "Component.h"

interface ICollider abstract : public IComponent {
public:
	ICollider(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const;
	bool CheckCollision(std::shared_ptr<ICollider> pOther) const;

	const BoundingOrientedBox& GetOBBWorld() const { return m_xmOBBWorld; }
	const BoundingBox GetAABBFromOBBWorld() const;

private:
	void MergeOBB(std::shared_ptr<IGameObject> pObj);

protected:
	BoundingOrientedBox m_xmOBBOrigin;
	BoundingOrientedBox m_xmOBBWorld;

};

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

class StaticCollider : public ICollider {
public:
	StaticCollider(std::shared_ptr<IGameObject> pOwner);
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

class DynamicCollider : public ICollider {
public:
	DynamicCollider(std::shared_ptr<IGameObject> pOwner);
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// Component Templates

template <>
struct ComponentIndex<ICollider> {
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
