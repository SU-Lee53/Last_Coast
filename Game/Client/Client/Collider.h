#pragma once
#include "Component.h"
#include "BoundingCapsule.h"

interface ICollider abstract : public IComponent {
	friend class CollisionResult;

public:
	ICollider(std::shared_ptr<IGameObject> pOwner);

	virtual bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const;
	virtual bool IsInAABB(const BoundingBox& xmAABB) const;
	virtual bool IsInOBB(const BoundingOrientedBox& xmOBB) const;
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
	virtual bool IsInAABB(const BoundingBox& xmFrustumInWorld) const override;
	virtual bool IsInOBB(const BoundingOrientedBox& xmFrustumInWorld) const override;
	virtual bool CheckCollision(std::shared_ptr<ICollider> pOther) const override;

private:
	BoundingCapsule m_CapsuleOrigin;
	BoundingCapsule m_CapsuleWorld;

};

//////////////////////////////////////////////////////////////////////////////////////
// MeshCollider
// UCX_ 콜리전 메시 기반 충돌체.
// 1차: OBB broad-phase, 2차: 삼각형 narrow-phase (vs PlayerCollider capsule)

class MeshCollider : public ICollider {
public:
	MeshCollider(std::shared_ptr<IGameObject> pOwner, COLLISIONMESHINFO info);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

	virtual bool CheckCollision(std::shared_ptr<ICollider> pOther) const override;

	// 마지막 CheckCollision에서 계산된 contact 정보 (삼각형 face normal 기반)
	Vector3 GetContactNormal() const { return m_v3LastContactNormal; }
	float   GetContactDepth()  const { return m_fLastContactDepth; }

private:
	bool CheckCapsuleVsTriangles(const BoundingCapsule& capsule, Vector3& outNormal, float& outDepth) const;

private:
	std::vector<Vector3>	m_v3BakedVertices;	// world space, baked in Initialize()
	std::vector<uint32>		m_unIndices;
	std::string				m_strType;

	mutable Vector3	m_v3LastContactNormal = Vector3{ 0.f, 1.f, 0.f };
	mutable float	m_fLastContactDepth = 0.f;
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

template <>
struct ComponentIndex<MeshCollider> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::COLLIDER;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::COLLIDER);
};
