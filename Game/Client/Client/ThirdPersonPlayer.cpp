#include "pch.h"
#include "ThirdPersonPlayer.h"
#include "ThirdPersonCamera.h"

ThirdPersonPlayer::ThirdPersonPlayer()
{
}

ThirdPersonPlayer::~ThirdPersonPlayer()
{
}

void ThirdPersonPlayer::Initialize()
{
	if (!m_bInitialized) {
		// Camera
		m_pCamera = std::make_shared<ThirdPersonCamera>();
		m_pCamera->SetViewport(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight, 0.f, 1.f);
		m_pCamera->SetScissorRect(0, 0, WinCore::sm_dwClientWidth, WinCore::sm_dwClientHeight);
		m_pCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, -15.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 500_m, (WinCore::sm_dwClientWidth / WinCore::sm_dwClientHeight), 60.0f);
		m_pCamera->SetOwner(shared_from_this());

		// Model
		auto pModel = MODEL->Get("Ch33_nonPBR")->CopyObject<GameObject>();
		pModel->GetTransform().Rotate(Vector3::Up, -90.f);
		SetChild(pModel);

		// AnimationController
		m_pAnimationController = std::make_shared<PlayerAnimationController>();
	}

	GameObject::Initialize();
}

void ThirdPersonPlayer::ProcessInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);

	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		m_bMouseInUse = !m_bMouseInUse;
	}

	// Camera Rotate
	if (m_bMouseInUse) {
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

		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		pThirdPersonCamera->AddYaw(ptDelta.x * m_fMouseSensitivity);
		pThirdPersonCamera->AddPitch(ptDelta.y * m_fMouseSensitivity);


		::SetCursorPos(nScreenCenterX, nScreenCenterY);

		// Aim
		if (INPUT->GetButtonDown(VK_RBUTTON)) {
			m_bAiming = true;
			pThirdPersonCamera->EnterAimMode();
			m_pAnimationController->GetMontage()->PlayMontage("Rifle Aiming Idle");
		}
		if (INPUT->GetButtonUp(VK_RBUTTON)) {
			m_bAiming = false;
			pThirdPersonCamera->LeaveAimMode();
			m_pAnimationController->GetMontage()->StopMontage();
		}

		// Fire
		if (INPUT->GetButtonPressed(VK_LBUTTON) && m_bAiming) {
			m_pAnimationController->GetMontage()->JumpToSection("Rifle Fire");
		}
	}

	Vector3 v3MoveDirection{};
	bool bMoved = false;

	// Move
	if (INPUT->GetButtonPressed('W')) {
		v3MoveDirection += pThirdPersonCamera->GetForwardXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('S')) {
		v3MoveDirection += -pThirdPersonCamera->GetForwardXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('A')) {
		v3MoveDirection += -pThirdPersonCamera->GetRightXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('D')) {
		v3MoveDirection += pThirdPersonCamera->GetRightXZ();
		bMoved = true;
	}

	// 테스트용 나중에 떼버릴것
	if (INPUT->GetButtonPressed('E')) {
		v3MoveDirection += m_Transform.GetUp();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('Q')) {
		v3MoveDirection += -m_Transform.GetUp();
		bMoved = true;
	}

	// Run
	if (INPUT->GetButtonPressed(VK_LSHIFT)) {
		m_bRunning = true;
	}
	else {
		m_bRunning = false;
	}

	if (bMoved) {
		v3MoveDirection.Normalize();
		m_fMoveSpeed += 0.5 * m_fAcceleration * m_fFriction;
		float fMaxSpeed = m_bRunning ? m_fMaxMoveSpeed * 2 : m_fMaxMoveSpeed;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, fMaxSpeed);
		m_Transform.Move(v3MoveDirection, m_fMoveSpeed * DT);

		// 플레이어가 이동 방향을 바라보도록 돌린다
		float fYaw = std::atan2f(v3MoveDirection.x, v3MoveDirection.z);
		m_Transform.SetRotation(0.f, fYaw, 0.f);
	}
	else {
		m_fMoveSpeed -= 0.5 * m_fAcceleration * m_fFriction;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, m_fMaxMoveSpeed);
	}
}

void ThirdPersonPlayer::Update()
{
	if (m_bAiming) {
		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		Vector3 v3LookDirection = pThirdPersonCamera->GetForwardXZ();
		float fYaw = std::atan2f(v3LookDirection.x, v3LookDirection.z);
		m_Transform.SetRotation(0.f, fYaw, 0.f);
	}

	Player::Update();
}

