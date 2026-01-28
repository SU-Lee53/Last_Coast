#include "pch.h"
#include "StaticObject.h"

void StaticObject::Initialize()
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

	if (m_pParent.expired() && !GetComponent<ICollider>()) {
		// Collider 의 경우 계층 변환의 자식 전파가 우선 필요하므로 마지막에 추가하고 Initialize
		AddComponent<StaticCollider>();
		GetComponent<StaticCollider>()->Initialize();
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
