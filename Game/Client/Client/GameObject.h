#pragma once
#include "Component.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "AnimationController.h"
#include "Collider.h"

struct MESHLOADINFO;
struct MATERIALLOADINFO;
struct CollisionResult;

class GameObject : public std::enable_shared_from_this<GameObject> {
	friend class ModelManager;

public:
	GameObject();
	~GameObject();

public:
	virtual void Initialize();
	virtual void ProcessInput() {}
	virtual void Update();

	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);
	virtual void RenderImmediate(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle);

public:
	template<ComponentType T>
	void AddComponent();

	template<ComponentType T, typename... Args>
	void AddComponent(Args&&... args);

	template<ComponentType T, typename... Args>
	void AddComponent(std::shared_ptr<T> pComponent);
	
	void SetBound(const Vector3& v3Center, const Vector3& v3Extents);
	void SetParent(std::shared_ptr<GameObject> pParent);
	void SetChild(std::shared_ptr<GameObject> pChild);
	void SetFrameName(const std::string& strFrameName);

	template<ComponentType T>
	std::shared_ptr<T> GetComponent() const;
	std::shared_ptr<Transform> GetTransform();
	const Matrix& GetWorldMatrix() const { return GetComponent<Transform>()->GetWorldMatrix(); }

	std::shared_ptr<GameObject> GetParent() const { return m_pParent.lock(); }
	const std::vector<std::shared_ptr<GameObject>>& GetChildren() { return m_pChildren; }

	const std::vector<Bone>& GetBones() const { return m_Bones; }
	size_t GetRootBoneIndex() const { return m_nRootBoneIndex; }
	int FindBoneIndex(const std::string& strBoneName) const;

public:
	virtual void OnBeginCollision(const CollisionResult& collisionResult) {};
	virtual void OnWhileCollision(const CollisionResult& collisionResult) {};
	virtual void OnEndCollision(const CollisionResult& collisionResult) {};

public:
	std::shared_ptr<GameObject> FindFrame(const std::string& strFrameName);
	std::shared_ptr<GameObject> FindMeshedFrame(const std::string& strFrameName);

	template<typename T>
	std::shared_ptr<T> CopyObject(std::shared_ptr<GameObject> pParent = nullptr) const;

protected:
	std::string m_strFrameName;

	std::vector<Bone> m_Bones;
	size_t m_nRootBoneIndex = 0;

	std::array<std::shared_ptr<IComponent>, std::to_underlying(COMPONENT_TYPE::COUNT)> m_pComponents = {};

	uint64 m_unGameObjectRuntimeID;

protected:
	std::weak_ptr<GameObject> m_pParent;
	std::vector<std::shared_ptr<GameObject>> m_pChildren = {};
	
protected:
	bool m_bInitialized = false;

public:
	template<typename T> requires std::derived_from<T, GameObject>
	static std::shared_ptr<T> CopyObject(std::shared_ptr<T> srcObject, std::shared_ptr<GameObject> pParent = nullptr);
	
	static uint64 g_GameObjectIDBase;

};

template<ComponentType T>
inline void GameObject::AddComponent()
{
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)] = std::make_shared<T>(shared_from_this());
}

template<ComponentType T, typename... Args>
inline void GameObject::AddComponent(Args&&... args)
{
	m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)] = std::make_shared<T>(shared_from_this(), std::forward<Args>(args)...);
}

template<ComponentType T, typename ...Args>
inline void GameObject::AddComponent(std::shared_ptr<T> pComponent)
{
	if (pComponent->IsOwnerExpired() || pComponent->GetOwner().get() != this) {
		pComponent->SetOwner(shared_from_this());
	}

	m_pComponents[ComponentIndex<T>::componentType] = pComponent;
}

template<ComponentType T>
inline 	std::shared_ptr<T> GameObject::GetComponent() const
{
	return static_pointer_cast<T>(m_pComponents[std::to_underlying(ComponentIndex<T>::componentType)]);
}

template<typename T>
std::shared_ptr<T> GameObject::CopyObject(std::shared_ptr<GameObject> pParent) const
{
	std::shared_ptr<T> pClone = std::make_shared<T>();
	pClone->m_strFrameName = m_strFrameName;

	for (int32 i = 0; i < std::to_underlying(COMPONENT_TYPE::COUNT); ++i) {
		if (m_pComponents[i]) {
			pClone->m_pComponents[i] = m_pComponents[i]->Copy(pClone);
		}
	}

	pClone->m_Bones = m_Bones;
	pClone->m_pParent = pParent;
	pClone->m_bInitialized = m_bInitialized;
	
	pClone->m_pChildren.reserve(m_pChildren.size());
	for (auto pChild : m_pChildren) {
		std::shared_ptr<decltype(pChild)::element_type> pChildClone = pChild->CopyObject<decltype(pChild)::element_type>(pClone);
		pClone->m_pChildren.push_back(pChildClone);
	}

	return pClone;
}

template<typename T> requires std::derived_from<T, GameObject>
inline std::shared_ptr<T> GameObject::CopyObject(std::shared_ptr<T> srcObject, std::shared_ptr<GameObject> pParent)
{
	std::shared_ptr<T> pClone = std::make_shared<T>();
	pClone->m_strFrameName = srcObject->m_strFrameName;

	for (int i = 0; i < COMPONENT_TYPE_COUNT; ++i) {
		if (m_pComponents[i]) {
			pClone->m_pComponents[i] = m_pComponents[i]->Copy(pClone);
		}
	}

	pClone->m_Bones = srcObject->m_Bones;
	pClone->m_pParent = pParent;
	pClone->m_bInitialized = srcObject->m_bInitialized;

	pClone->m_pParent = pParent;

	pClone->m_pChildren.reserve(srcObject->m_pChildren.size());
	for (auto pChild : srcObject->m_pChildren) {
		std::shared_ptr<decltype(pChild)::element_type> pChildClone = CopyObject(pChild, pClone);
		pClone->m_pChildren.push_back(pChildClone);
	}

	return pClone;
}
