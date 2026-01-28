#include "pch.h"
#include "GameObject.h"
#include "Transform.h"

uint64 IGameObject::g_GameObjectIDBase = 0;

IGameObject::IGameObject()
{
	m_unGameObjectRuntimeID = g_GameObjectIDBase++;
}

IGameObject::~IGameObject()
{
}

void IGameObject::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
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

void IGameObject::RenderImmediate(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle)
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

void IGameObject::SetParent(std::shared_ptr<IGameObject> pParent)
{
	if (pParent) {
		m_pParent = pParent;
	}
}

void IGameObject::SetChild(std::shared_ptr<IGameObject> pChild)
{
	if (pChild)
	{
		pChild->m_pParent = shared_from_this();
		m_pChildren.push_back(pChild);
	}


	// Skeleton 과 AnimationController 가 있다면 반드시 Root 로 옮겨와야 함
	if (m_pParent.expired()) {
		if (pChild->GetComponent<Skeleton>()) {
			MoveComponent<Skeleton>(pChild);
		}

		if (pChild->GetComponent<AnimationController>()) {
			MoveComponent<AnimationController>(pChild);
		}
	}
}

void IGameObject::SetName(const std::string& strFrameName)
{
	m_strFrameName = strFrameName;
}

std::shared_ptr<Transform> IGameObject::GetTransform()
{
	if (!m_pComponents[std::to_underlying(COMPONENT_TYPE::TRANSFORM)]) {
		AddComponent<Transform>();
	}

	return std::static_pointer_cast<Transform>(m_pComponents[std::to_underlying(COMPONENT_TYPE::TRANSFORM)]);
}

std::shared_ptr<IGameObject> IGameObject::FindFrame(const std::string& strFrameName)
{
	std::shared_ptr<IGameObject> pFrameObject;
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

std::shared_ptr<IGameObject> IGameObject::FindMeshedFrame(const std::string& strFrameName)
{
	std::shared_ptr<IGameObject> pFrameObject;
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
