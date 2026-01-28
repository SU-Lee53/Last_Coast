#include "pch.h"
#include "NodeObject.h"

void NodeObject::Initialize()
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
}

void NodeObject::ProcessInput()
{
}

void NodeObject::PreUpdate()
{
}

void NodeObject::Update()
{
}

void NodeObject::PostUpdate()
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
