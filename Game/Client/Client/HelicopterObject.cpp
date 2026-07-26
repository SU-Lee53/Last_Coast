#include "pch.h"
#include "HelicopterObject.h"

HelicopterObject::~HelicopterObject()
{
	SOUND->Stop(m_pSoundChannel);
}

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

void HelicopterObject::PostUpdate()
{
	DynamicObject::PostUpdate();
	if (m_pSoundChannel) {
		if (SOUND->IsPlaying(m_pSoundChannel)) {
			SOUND->SetChannelPosition(m_pSoundChannel, GetTransform()->GetPosition());
		}
		else {
			m_pSoundChannel = nullptr;
		}
	}
}

void HelicopterObject::PlaySound(const std::string& strSoundName)
{
	SOUND->Stop(m_pSoundChannel);
	m_pSoundChannel = SOUND->PlayAt(strSoundName, GetTransform()->GetPosition());
}
