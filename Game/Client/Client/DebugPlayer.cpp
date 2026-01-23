#include "pch.h"
#include "DebugPlayer.h"
#include "FirstPersonCamera.h"

DebugPlayer::DebugPlayer()
{
}

DebugPlayer::~DebugPlayer()
{
}

void DebugPlayer::Initialize()
{
	if (!m_bInitialized) {
		// Camera
		m_pCamera = std::make_shared<FirstPersonCamera>();
		m_pCamera->SetViewport(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight, 0.f, 1.f);
		m_pCamera->SetScissorRect(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight);
		m_pCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, -15.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_pCamera->GenerateProjectionMatrix(1.01_cm, 500_m, (WinCore::sm_dwClientWidth / WinCore::sm_dwClientHeight), 60.0f);
		m_pCamera->SetOwner(shared_from_this());
	}

	GameObject::Initialize();
}

void DebugPlayer::ProcessInput()
{
	auto pTransform = GetTransform();

	if (INPUT->GetButtonPressed(VK_RBUTTON)) {
		HWND hWnd = ::GetActiveWindow();

		::SetCursor(NULL);

		RECT rtClientRect;
		::GetClientRect(hWnd, &rtClientRect);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.left);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.right);

		int nScreenCenterX = 0, nScreenCenterY = 0;
		nScreenCenterX = rtClientRect.left + WinCore::sm_dwClientWidth / 2;
		nScreenCenterY = rtClientRect.top + WinCore::sm_dwClientHeight / 2;

		POINT ptCursorPos;
		::GetCursorPos(&ptCursorPos);

		POINT ptDelta{ (ptCursorPos.x - nScreenCenterX), (ptCursorPos.y - nScreenCenterY) };

		m_fYaw += (float)ptDelta.x * m_fMouseSensitivity;
		m_fPitch += (float)ptDelta.y * m_fMouseSensitivity;

		// Pitch 제한 -> 화면 뒤집히지 않도록
		if (m_fPitch > XMConvertToRadians(89.0f))
			m_fPitch = XMConvertToRadians(89.0f);
		if (m_fPitch < XMConvertToRadians(-89.0f))
			m_fPitch = XMConvertToRadians(-89.0f);

		pTransform->SetRotation(m_fPitch, m_fYaw, 0.f);

		::SetCursorPos(nScreenCenterX, nScreenCenterY);
	}

	Vector3 v3MoveDirection{};

	if (INPUT->GetButtonPressed('W')) {
		v3MoveDirection += pTransform->GetLook();
	}
	if (INPUT->GetButtonPressed('S')) {
		v3MoveDirection += -pTransform->GetLook();
	}
	if (INPUT->GetButtonPressed('A')) {
		v3MoveDirection += -pTransform->GetRight();
	}
	if (INPUT->GetButtonPressed('D')) {
		v3MoveDirection += pTransform->GetRight();
	}
	if (INPUT->GetButtonPressed('E')) {
		v3MoveDirection += pTransform->GetUp();
	}
	if (INPUT->GetButtonPressed('Q')) {
		v3MoveDirection += -pTransform->GetUp();
	}

	v3MoveDirection.Normalize();
	pTransform->Move(v3MoveDirection, 10_m * DT);

}

