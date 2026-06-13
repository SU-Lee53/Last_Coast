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
	virtual BoundingBox GetAABBFromOBBWorld() const;

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

	const BoundingCapsule& GetCapsuleOrigin() const { return m_CapsuleOrigin; }
	const BoundingCapsule& GetCapsuleWorld() const { return m_CapsuleWorld; }

	virtual bool IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const override;
	virtual bool IsInAABB(const BoundingBox& xmFrustumInWorld) const override;
	virtual bool IsInOBB(const BoundingOrientedBox& xmFrustumInWorld) const override;
	virtual bool CheckCollision(std::shared_ptr<ICollider> pOther) const override;

	virtual BoundingBox GetAABBFromOBBWorld() const override;

	void ResetCharacterModelCollision();

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

	// 마지막 CheckCollision에서 계산된 contact 목록 (바닥·벽 카테고리별 최대 depth)
	const std::vector<std::pair<Vector3, float>>& GetContacts() const { return m_LastContacts; }

	// 캡슐이 이 메시와 교차하는지 여부만 반환 (step-up 테스트용)
	bool TestCapsule(const BoundingCapsule& capsule) const;

private:
	// 바닥(normal.y > 0.7)과 벽/경사 카테고리를 분리해 각 최대 depth contact를 outContacts에 추가
	bool CheckCapsuleVsTriangles(const BoundingCapsule& capsule,
		std::vector<std::pair<Vector3, float>>& outContacts) const;

private:
	std::vector<Vector3>	m_v3BakedVertices;	// world space, baked in Initialize()
	std::vector<uint32>		m_unIndices;
	std::string				m_strType;

	mutable std::vector<std::pair<Vector3, float>> m_LastContacts;
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
