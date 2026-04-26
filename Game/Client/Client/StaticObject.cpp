#include "pch.h"
#include "StaticObject.h"
#include "Collider.h"

void StaticObject::Initialize()
{
	if (!m_bInitialized) {
		// 필수 Component(Transform) 우선 생성
		if (!GetComponent<Transform>()) {
			AddComponent<Transform>();
		}

		// Collider는 계층 변환 전파 후 초기화해야 하므로 루프에서 제외
		for (auto& component : m_pComponents) {
			if (component && !std::dynamic_pointer_cast<ICollider>(component)) {
				component->Initialize();
			}
		}

		GetTransform()->Update();

		m_bInitialized = true;
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	if (m_pParent.expired()) {
		// Collider 의 경우 계층 변환의 자식 전파가 우선 필요하므로 마지막에 Initialize
		auto pCollider = GetComponent<ICollider>();
		if (!pCollider) {
			AddComponent<StaticCollider>();
			pCollider = GetComponent<ICollider>();
		}
		pCollider->Initialize();
	}
}

void StaticObject::ProcessInput()
{
}

void StaticObject::PreUpdate()
{
}

void StaticObject::Update()
{
	for (const auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void StaticObject::PostUpdate()
{
	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->PostUpdate();
	}
}
