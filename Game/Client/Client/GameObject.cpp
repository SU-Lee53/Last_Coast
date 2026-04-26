#include "pch.h"
#include "GameObject.h"
#include "Transform.h"
#include "TerrainObject.h"

uint64 IGameObject::g_GameObjectIDBase = 0;

IGameObject::IGameObject(OBJECT_MOBILITY_TYPE eType) : m_eObjectType{ eType }
{
	m_unGameObjectRuntimeID = g_GameObjectIDBase++;
}

IGameObject::~IGameObject()
{
}

void IGameObject::Render()
{
	// TODO : PrepareRender Logic Here
	if (auto p = GetComponent<MeshRenderer>()) {
		RENDER->Add(shared_from_this());
	}

	for (auto& pChild : m_pChildren) {
		pChild->Render();
	}
}

void IGameObject::SetRoot(std::shared_ptr<IGameObject> pRoot)
{
	if (pRoot) {
		m_pParent = pRoot;
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
	if (pChild) {
		pChild->m_pParent = shared_from_this();
		m_pChildren.push_back(pChild);
	}

	if (m_pParent.expired()) {	// Root 에 자식추가 -> 자식들에게 새로운 Root 를 전달 
		pChild->PropagateRoot(shared_from_this());
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

void IGameObject::PropagateRoot(std::shared_ptr<IGameObject> pRoot)
{
	m_pRoot = pRoot;
	for (auto pChild : m_pChildren) {
		pChild->PropagateRoot(pRoot);
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


void IGameObject::ShowControlImGui()
{
	std::string& strTreeNodeName = m_strFrameName;

	if (ImGui::TreeNode(strTreeNodeName.c_str())) {
		constexpr auto nComponents = std::to_underlying(COMPONENT_TYPE::COUNT);
		for (int i = 0; i < nComponents; ++i) {
			if (m_pComponents[i]) {
				m_pComponents[i]->ShowControlImGui();
			}
		}

		for (const auto& pChild : m_pChildren) {
			pChild->ShowControlImGui();
		}

		ImGui::TreePop();
	}
}
