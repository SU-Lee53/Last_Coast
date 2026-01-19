#pragma once
#include "Component.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "AnimationController.h"

struct MESHLOADINFO;
struct MATERIALLOADINFO;

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
	void RenderDirectly(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle);

public:

	template<ComponentType T>
	void AddComponent();

	template<ComponentType T, typename... Args>
	void AddComponent(Args&&... args);

	template<ComponentType T, typename... Args>
	void AddComponent(std::shared_ptr<T> pComponent);


	// Create New
	template<typename T, typename... Args> requires std::derived_from<T, IMeshRenderer>
	void SetMeshRenderer(Args&&... args);
	
	// Set Existed
	void SetMeshRenderer(std::shared_ptr<IMeshRenderer> pMeshRenderer) { m_pMeshRenderer = pMeshRenderer; }
	
	void SetBound(const Vector3& v3Center, const Vector3& v3Extents);
	void SetParent(std::shared_ptr<GameObject> pParent);
	void SetChild(std::shared_ptr<GameObject> pChild);
	void SetFrameName(const std::string& strFrameName);
	void SetAnimationController(std::shared_ptr<AnimationController> pController) { m_pAnimationController = pController; }

	template<ComponentType T>
	std::shared_ptr<T> GetComponent();
	std::shared_ptr<GameObject> GetParent() const { return m_pParent.lock(); }
	std::shared_ptr<IMeshRenderer> GetMeshRenderer() const { return m_pMeshRenderer; }
	Transform& GetTransform() { return m_Transform; }
	const Matrix& GetWorldMatrix() const { return m_Transform.GetWorldMatrix(); }
	const std::vector<Bone>& GetBones() const { return m_Bones; }
	size_t GetRootBoneIndex() const { return m_nRootBoneIndex; }
	int FindBoneIndex(const std::string& strBoneName) const;
	std::shared_ptr<AnimationController> GetAnimationController() const { return m_pAnimationController; }

	void MergeBoundingBox(BoundingOrientedBox* pOBB);

public:
	std::shared_ptr<GameObject> FindFrame(const std::string& strFrameName);
	std::shared_ptr<GameObject> FindMeshedFrame(const std::string& strFrameName);

	template<typename T>
	std::shared_ptr<T> CopyObject(std::shared_ptr<GameObject> pParent = nullptr) const;

protected:
	bool m_bInitialized = false;
	std::string m_strFrameName;
	Transform m_Transform{};
	std::shared_ptr<IMeshRenderer> m_pMeshRenderer = nullptr;

	// 아래 2가지 애니메이션과 관련된 것들은 반드시 Root에 보관되어야 함
	std::vector<Bone> m_Bones;
	size_t m_nRootBoneIndex = 0;
	std::shared_ptr<AnimationController> m_pAnimationController = nullptr;

	std::array<std::shared_ptr<Component>, COMPONENT_TYPE_COUNT> m_pComponents = {};

protected:
	// Frame Hierarchy
	// 단일 모델의 경우에는 해당 GameObject 이 Root 이고 parent 와 child 는 없음
	// 계층 모델의 경우 Root 의 Mesh 는 비어있고, 자식들을 순회하며 Mesh 들을 렌더링 함

	std::weak_ptr<GameObject> m_pParent;
	std::vector<std::shared_ptr<GameObject>> m_pChildren = {};
	
protected:
	// Bounding Volume
	BoundingOrientedBox m_xmOBB;

public:
	template<typename T> requires std::derived_from<T, GameObject>
	static std::shared_ptr<T> CopyObject(std::shared_ptr<T> srcObject, std::shared_ptr<GameObject> pParent = nullptr);

};

template<typename T, typename... Args> requires std::derived_from<T, IMeshRenderer>
inline void GameObject::SetMeshRenderer(Args&&... args)
{
	m_pMeshRenderer = std::make_shared<T>(std::forward<Args>(args)...);
}

template<ComponentType T>
inline void GameObject::AddComponent()
{
	m_pComponents[ComponentIndex<T>::componentType] = std::make_shared<T>(shared_from_this());
}

template<ComponentType T, typename... Args>
inline void GameObject::AddComponent(Args&&... args)
{
	m_pComponents[ComponentIndex<T>::componentType] = std::make_shared<T>(std::forward<Args>(args)...);
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
inline 	std::shared_ptr<T> GameObject::GetComponent()
{
	return static_pointer_cast<T>(m_pComponents[ComponentIndex<T>::componentType]);
}

template<typename T>
std::shared_ptr<T> GameObject::CopyObject(std::shared_ptr<GameObject> pParent) const
{
	std::shared_ptr<T> pClone = std::make_shared<T>();
	pClone->m_strFrameName = m_strFrameName;
	pClone->m_Transform = m_Transform;

	for (int i = 0; i < COMPONENT_TYPE_COUNT; ++i) {
		pClone->m_pComponents[i] = m_pComponents[i];
		pClone->m_pComponents[i]->SetOwner(pClone);
	}

	pClone->m_pMeshRenderer = m_pMeshRenderer;
	pClone->m_Bones = m_Bones;
	pClone->m_pAnimationController = m_pAnimationController;
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
	pClone->m_Transform = srcObject->m_Transform;

	// 수정필요 (10.21)
	// 독립된 객체이되 내부 내용물은 같아야 하고, Owner 는 새로 지정되어야 함
	// 원본 Component 의 타입이 보존되어야 함
	// 아니면 MeshRenderer 가 Component 가 아니게 해버리는 방법도 있긴함 -> Owner 개념이 없어지니 속편하게 복사 가능
	// Component 가 얼마나 늘어날지 아직 모르는 상황에서 전부다 Copy 를 구현하는것은 매우 무리가 있을듯
	// 애시당초 Mesh 를 공유하는게 "인스턴싱"인 상황에 Owner 개념을 부여하여 고유하게 만드는게 과연 옳은지?
	for (int i = 0; i < COMPONENT_TYPE_COUNT; ++i) {
		pClone->m_pComponents[i] = srcObject->m_pComponents[i];
		pClone->m_pComponents[i]->SetOwner(pClone);
	}
	
	// 수정완 (10.21)
	pClone->m_pMeshRenderer = srcObject->m_pMeshRenderer;
	pClone->m_pComponents = srcObject->m_pComponents;
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
