#include "pch.h"
#include "GameObject.h"
#include "Transform.h"

uint64 GameObject::g_GameObjectIDBase = 0;

GameObject::GameObject()
{
	m_unGameObjectRuntimeID = g_GameObjectIDBase++;
}

GameObject::~GameObject()
{
}

void GameObject::Initialize()
{
	if (!m_bInitialized) {
		// 필수 Component(Transform) 우선 생성
		if (!GetComponent<Transform>()) {
			AddComponent<Transform>();
		}

		for (auto& component : m_pComponents) {
			if (component) {
				component->Initialize();
			}
		}

		GetTransform()->Update();

		m_bInitialized = true;
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	if (m_pParent.expired() && !GetComponent<Collider>()) {
		// Collider 의 경우 계층 변환의 자식 전파가 우선 필요하므로 마지막에 추가하고 Initialize
		// 기본은 StaticCollider
		AddComponent<StaticCollider>();
		GetComponent<StaticCollider>()->Initialize();
	}
}

void GameObject::Update()
{
	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	// TODO : Render Logic Here
	if (GetComponent<AnimationController>()) {
		RENDER->AddAnimatedObject(shared_from_this());
		return;
	}

	for (auto& pChild : m_pChildren) {
		pChild->Render(pd3dCommandList);
	}
}

void GameObject::RenderImmediate(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
{
	auto pMeshRenderer = GetComponent<MeshRenderer>();
	if (pMeshRenderer) {
		int nInstanceBase = -1;
		Matrix mtxWorld = GetTransform()->GetWorldMatrix();
		pMeshRenderer->Render(pd3dDevice, pd3dCommandList, descHandle, 1, nInstanceBase, mtxWorld);
	}

	for (auto& pChild : m_pChildren) {
		pChild->RenderImmediate(pd3dDevice, pd3dCommandList, descHandle);
	}
}

void GameObject::SetParent(std::shared_ptr<GameObject> pParent)
{
	if (pParent) {
		m_pParent = pParent;
	}
}

void GameObject::SetChild(std::shared_ptr<GameObject> pChild)
{
	if (pChild)
	{
		pChild->m_pParent = shared_from_this();
		m_pChildren.push_back(pChild);
	}

	if (m_pParent.expired() && pChild->m_Bones.size() != 0) {
		m_Bones = std::move(pChild->m_Bones);
	}
}

void GameObject::SetFrameName(const std::string& strFrameName)
{
	m_strFrameName = strFrameName;
}

std::shared_ptr<Transform> GameObject::GetTransform()
{
	if (!m_pComponents[std::to_underlying(COMPONENT_TYPE::TRANSFORM)]) {
		AddComponent<Transform>();
	}

	return std::static_pointer_cast<Transform>(m_pComponents[std::to_underlying(COMPONENT_TYPE::TRANSFORM)]);
}

int GameObject::FindBoneIndex(const std::string& strBoneName) const
{
	if (m_Bones.size() == 0) {
		return -1;
	}
	
	std::vector<const Bone*> DFSStack;
	DFSStack.reserve(m_Bones.size());
	DFSStack.push_back(&m_Bones[m_nRootBoneIndex]);

	const Bone* pCurBone = nullptr;
	while (true) {
		if (DFSStack.size() == 0) {
			break;
		}

		pCurBone = DFSStack.back();
		DFSStack.pop_back();

		if (pCurBone->strBoneName == strBoneName) {
			return pCurBone->nIndex;
		}

		for (int i = 0; i < pCurBone->nChildren; ++i) {
			DFSStack.push_back(&m_Bones[pCurBone->nChilerenIndex[i]]);
		}
	}

	return -1;
}

std::shared_ptr<GameObject> GameObject::FindFrame(const std::string& strFrameName)
{
	std::shared_ptr<GameObject> pFrameObject;
	if (strFrameName == m_strFrameName) {
		return shared_from_this();
	}

	for (auto& pChild : m_pChildren) {
		if (pFrameObject = pChild->FindFrame(strFrameName)) {
			return pFrameObject;
		}
	}

	return nullptr;
}

std::shared_ptr<GameObject> GameObject::FindMeshedFrame(const std::string& strFrameName)
{
	std::shared_ptr<GameObject> pFrameObject;
	if (strFrameName == m_strFrameName && GetComponent<MeshRenderer>()) {
		return shared_from_this();
	}

	for (auto& pChild : m_pChildren) {
		if (pFrameObject = pChild->FindMeshedFrame(strFrameName)) {
			return pFrameObject;
		}
	}

	return nullptr;
}
