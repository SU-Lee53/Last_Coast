#pragma once
#include "Component.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Collider.h"
#include "Skeleton.h"
#include "AnimationController.h"

struct MESHLOADINFO;
struct MATERIALLOADINFO;
struct CollisionResult;

enum class OBJECT_MOBILITY_TYPE { STATIC, DYNAMIC, UNKNOWN };

interface IGameObject abstract : public std::enable_shared_from_this<IGameObject> {
	friend class ModelManager;
public:

public:
	IGameObject(OBJECT_MOBILITY_TYPE eType);
	virtual ~IGameObject();

public:
	virtual void Initialize() = 0;
	virtual void ProcessInput() = 0;

	virtual void PreUpdate() = 0;
	virtual void Update() = 0;
	virtual void PostUpdate() = 0;
	virtual void PostAnimationUpdate() {}

	virtual void Render();
	virtual void AddToQueue(OUT std::vector<IGameObject*>& pRenderQueue);

	// 캐릭터(플레이어) 여부 — 탈출 컷씬에서 전 플레이어를 숨길 때 렌더 제외용
	virtual bool IsCharacter() const { return false; }

public:
	template<ComponentType T>
	void AddComponent();

	template<ComponentType T, typename... Args>
	void AddComponent(Args&&... args);

	template<ComponentType T, typename... Args>
	void AddComponent(std::shared_ptr<T> pComponent);
	
	void SetRoot(std::shared_ptr<IGameObject> pRoot);
	void SetParent(std::shared_ptr<IGameObject> pParent);
	void SetChild(std::shared_ptr<IGameObject> pChild);
	void SetName(const std::string& strFrameName);

	void SwapChild(size_t nIndex, std::shared_ptr<IGameObject> pNewChild);

	const std::string& GetName() const { return m_strFrameName; }

	template<ComponentType T>
	std::shared_ptr<T> GetComponent() const;

	template<ComponentType T>
	std::shared_ptr<T> GetComponentFromRoot() const;

	std::shared_ptr<Transform> GetTransform();
	const Matrix& GetWorldMatrix() const { return GetComponent<Transform>()->GetWorldMatrix(); }

	std::shared_ptr<IGameObject> GetParent() const { return m_pParent.lock(); }
	const std::vector<std::shared_ptr<IGameObject>>& GetChildren() { return m_pChildren; }

	const OBJECT_MOBILITY_TYPE GetObjectType() const { return m_eObjectType; }

public:
	virtual void OnBeginCollision(const CollisionResult& collisionResult) {};
	virtual void OnWhileCollision(const CollisionResult& collisionResult) {};
	virtual void OnEndCollision(const CollisionResult& collisionResult) {};
	virtual void OnTraceHit(const RayTraceHitResult& hitResult) {};

public:
	std::shared_ptr<IGameObject> FindFrame(const std::string& strFrameName);
	std::shared_ptr<IGameObject> FindFrameEndsWith(const std::string& strFrameName);
	std::shared_ptr<IGameObject> FindMeshedFrame(const std::string& strFrameName);

	template<typename T>
	std::shared_ptr<T> CopyObject(
		std::shared_ptr<IGameObject> pParent = nullptr, 
		std::shared_ptr<IGameObject> pRoot = nullptr) const;

protected:
	template<ComponentType T>
	void MoveComponent(std::shared_ptr<IGameObject> pFrom);

	void PropagateRoot(std::shared_ptr<IGameObject> pRoot);

public:
	void ShowControlImGui();

protected:
	std::string m_strFrameName;
	std::array<std::shared_ptr<IComponent>, std::to_underlying(COMPONENT_TYPE::COUNT)> m_pComponents = {};

	std::weak_ptr<IGameObject> m_pRoot;
	std::weak_ptr<IGameObject> m_pParent;
	std::vector<std::shared_ptr<IGameObject>> m_pChildren = {};
	
	Vector3 m_v3FloorPosition = Vector3(0, 0, 0);
	bool m_bInitialized = false;
	bool m_bGrounded = false;

	uint64 m_unGameObjectRuntimeID;

	OBJECT_MOBILITY_TYPE m_eObjectType;

public:
	static std::atomic_uint64_t g_GameObjectIDBase;

};

template<ComponentType T>
inline void IGameObject::AddComponent()
{
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)] = std::make_shared<T>(shared_from_this());
}

template<ComponentType T>
inline void IGameObject::MoveComponent(std::shared_ptr<IGameObject> pFrom)
{
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)] = std::move(pFrom->m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)]);
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)]->SetOwner(shared_from_this());
}

template<ComponentType T, typename... Args>
inline void IGameObject::AddComponent(Args&&... args)
{
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)] = std::make_shared<T>(shared_from_this(), std::forward<Args>(args)...);
}

template<ComponentType T, typename ...Args>
inline void IGameObject::AddComponent(std::shared_ptr<T> pComponent)
{
	if (pComponent->IsOwnerExpired() || pComponent->GetOwner().get() != this) {
		pComponent->SetOwner(shared_from_this());
	}

	m_pComponents[ComponentIndex<T>::componentType] = pComponent;
}

template<ComponentType T>
inline 	std::shared_ptr<T> IGameObject::GetComponent() const
{
	return static_pointer_cast<T>(m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)]);
}

template<ComponentType T>
inline std::shared_ptr<T> IGameObject::GetComponentFromRoot() const
{
	if (m_pRoot.expired()) {
		return GetComponent<T>();
	}
	return static_pointer_cast<T>(m_pRoot.lock()->GetComponent<T>());
}

template<typename T>
inline std::shared_ptr<T> IGameObject::CopyObject(std::shared_ptr<IGameObject> pParent, std::shared_ptr<IGameObject> pRoot) const
{
	std::shared_ptr<T> pClone = std::make_shared<T>();
	pClone->m_strFrameName = m_strFrameName;

	for (int32 i = 0; i < std::to_underlying(COMPONENT_TYPE::COUNT); ++i) {
		if (m_pComponents[i]) {
			pClone->m_pComponents[i] = m_pComponents[i]->Copy(pClone);
		}
	}

	pClone->m_pParent = pParent;

	if (!pRoot) {
		pRoot = pClone;
	}
	else {
		pClone->m_pRoot = pRoot;
	}

	pClone->m_bInitialized = m_bInitialized;
	
	pClone->m_pChildren.reserve(m_pChildren.size());
	for (auto pChild : m_pChildren) {
		std::shared_ptr<NodeObject> pChildClone = pChild->CopyObject<NodeObject>(pClone, pRoot);
		pClone->m_pChildren.push_back(pChildClone);
	}

	if (auto pSkeleton = pClone->GetComponent<Skeleton>()) {
		pSkeleton->ResetBoneNodes();
	}

	return pClone;
}
