#pragma once
#include "Component.h"
#include "BoundingCapsule.h"

interface ICollider abstract : public IComponent {
	friend class CollisionResult;

public:
	ICollider(std::shared_ptr<IGameObject> pOwner);

	virtual bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const;
	virtual bool CheckCollision(std::shared_ptr<ICollider> pOther) const;

	const BoundingOrientedBox& GetOBBWorld() const { return m_xmOBBWorld; }
	const BoundingBox GetAABBFromOBBWorld() const;

protected:
	void MergeOBB(std::shared_ptr<IGameObject> pObj, bool bFixInWorld);

protected:
	BoundingOrientedBox m_xmOBBOrigin;
	BoundingOrientedBox m_xmOBBWorld;

};

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

class StaticCollider : public ICollider {
public:
	StaticCollider(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

class DynamicCollider : public ICollider {
public:
	DynamicCollider(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;
};

//////////////////////////////////////////////////////////////////////////////////////
// PlayerCollider

class PlayerCollider : public ICollider {
public:
	PlayerCollider(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;

	const BoundingCapsule& GetCapsuleOrigin() { return m_CapsuleOrigin; }
	const BoundingCapsule& GetCapsuleWorld() { return m_CapsuleWorld; }

	virtual bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const override;
	virtual bool CheckCollision(std::shared_ptr<ICollider> pOther) const override;

private:
	BoundingCapsule m_CapsuleOrigin;
	BoundingCapsule m_CapsuleWorld;

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

template <>
struct ComponentIndex<PlayerCollider> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::COLLIDER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::COLLIDER);
};
