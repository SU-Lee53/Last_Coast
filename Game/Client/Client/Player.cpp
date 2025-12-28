#include "pch.h"
#include "Player.h"
#include "Camera.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::Initialize()
{
	if (!m_bInitialized) {
		// Camera
		m_pCamera = std::make_shared<Camera>();
		m_pCamera->SetViewport(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight, 0.f, 1.f);
		m_pCamera->SetScissorRect(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight);
		m_pCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, -15.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, (WinCore::sm_dwClientWidth / WinCore::sm_dwClientHeight), 60.0f);

		m_bInitialized = true;
	}

	GameObject::Initialize();
}

void Player::ProcessInput()
{
}

void Player::Update()
{
	if (m_pCamera) {
		m_pCamera->Update();
	}

	GameObject::Update();
}

void Player::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
}
