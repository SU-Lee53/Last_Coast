#include "pch.h"
#include "DynamicObject.h"

void DynamicObject::Initialize()
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
		AddComponent<DynamicCollider>();
		GetComponent<DynamicCollider>()->Initialize();
	}
}

void DynamicObject::ProcessInput()
{
}

void DynamicObject::PreUpdate()
{
}

void DynamicObject::Update()
{
}

void DynamicObject::PostUpdate()
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
