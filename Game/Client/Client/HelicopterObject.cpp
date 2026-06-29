#include "pch.h"
#include "HelicopterObject.h"

void HelicopterObject::Initialize()
{
	if (!GetComponent<Transform>()) {
		AddComponent<Transform>();
	}


	for (auto& component : m_pComponents) {
		if (component) {
			component->Initialize();
		}
	}

	auto pModel = MODEL->LoadOrGet("Gunship")->CopyObject<NodeObject>();
	pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
	SetChild(pModel);

	GetTransform()->Update();

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	AddComponent<PlayerCollider>();
	GetComponent<PlayerCollider>()->Initialize();

	m_pMainRotorFrame = FindFrame("Rotor");
	m_bInitialized = true;
}

void HelicopterObject::ProcessInput()
{
}

void HelicopterObject::Update()
{
	if (m_bRotorActive && m_pMainRotorFrame)
	{
		auto& pTransform = m_pMainRotorFrame->GetTransform();
		pTransform->Rotate(Vector3{ 0.0f, 10.f * DT, 0.0f });
	}

}
