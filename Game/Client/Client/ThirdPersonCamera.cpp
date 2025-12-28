#include "pch.h"
#include "ThirdPersonCamera.h"

//#define CAMERA_WITH_DELAY

ThirdPersonCamera::ThirdPersonCamera()
{
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

void ThirdPersonCamera::ProcessInput()
{
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

		SetRotation(m_fPitch, m_fYaw, 0.f);

		::SetCursorPos(nScreenCenterX, nScreenCenterY);

	}

	if (INPUT->GetButtonPressed('W')) {
		SOUND->Resume("spaceship_sfx");
		m_v3Position += m_v3Look * m_fPlayerSpeed * DT;
	}

	if (INPUT->GetButtonPressed('S')) {
		SOUND->Resume("spaceship_sfx");
		m_v3Position -= m_v3Look * m_fPlayerSpeed * DT;
	}

	if (INPUT->GetButtonPressed('D')) {
		SOUND->Resume("spaceship_sfx");
		m_v3Position += m_v3Right * m_fPlayerSpeed * DT;
	}

	if (INPUT->GetButtonPressed('A')) {
		SOUND->Resume("spaceship_sfx");
		m_v3Position -= m_v3Right * m_fPlayerSpeed * DT;
	}

	if (INPUT->GetButtonPressed(VK_SPACE)) {
		SOUND->Resume("spaceship_sfx");
		m_v3Position += m_v3Up * m_fPlayerSpeed * DT;
	}

	if (INPUT->GetButtonPressed('W') == false &&
		INPUT->GetButtonPressed('S') == false &&
		INPUT->GetButtonPressed('A') == false &&
		INPUT->GetButtonPressed('D') == false &&
		INPUT->GetButtonPressed(VK_SPACE) == false) 
	{

		SOUND->Pause("spaceship_sfx");
	}

}

void ThirdPersonCamera::Update()
{
#ifdef CAMERA_WITH_DELAY
	const Transform& ownerTransform = m_wpOwner.lock()->GetTransform();
	Vector3 v3Right = ownerTransform.GetRight();
	Vector3 v3Up = ownerTransform.GetUp();
	Vector3 v3Look = ownerTransform.GetLook();

	Matrix mtxRotate = Matrix::Identity;
	mtxRotate._11 = v3Right.x; mtxRotate._21 = v3Up.x; mtxRotate._31 = v3Look.x;
	mtxRotate._12 = v3Right.y; mtxRotate._22 = v3Up.y; mtxRotate._32 = v3Look.y;
	mtxRotate._13 = v3Right.z; mtxRotate._23 = v3Up.z; mtxRotate._33 = v3Look.z;

	Vector3 v3Offset = Vector3::Transform(m_v3Offset, mtxRotate);
	Vector3 v3Position = ownerTransform.GetPosition() + v3Offset;
	Vector3 v3Direction = v3Position - m_v3Position;
	float fLength = v3Direction.Length();
	v3Direction.Normalize();
	float fTimeLagScale = (m_fTimeLag) ? (1.f / m_fTimeLag) * DT : 1.0f;
	float fDistance = fLength * fTimeLagScale;
	if (fDistance > fLength) fDistance = fLength;
	if (fLength < 0.01f) fDistance = fLength;
	if (fDistance > 0) {
		m_v3Position = m_v3Position + (v3Direction * fDistance);
		Vector3 v3PlayerUp = ownerTransform.GetUp();
		v3PlayerUp.Normalize();
		SetLookAt(ownerTransform.GetPosition() + (v3PlayerUp * 3));
	}
#else
	// 11.01
	// TODO : 카메라가 Roll 을 따라가서는 안됨
	// 어떻게? -> 그냥 로직을 원본꺼랑 동일하게 해보자
	
	//	const Transform& ownerTransform = m_wpOwner.lock()->GetTransform();
	//	
	//	Vector3 v3Offset = Vector3::TransformNormal(m_v3Offset, ownerTransform.GetWorldMatrix());
	//	Vector3 v3Position = ownerTransform.GetPosition() + v3Offset;
	//	
	//	m_v3Position = v3Position;
	//	Vector3 v3PlayerUp = ownerTransform.GetUp();
	//	v3PlayerUp.Normalize();
	//	SetLookAt(ownerTransform.GetPosition() + (v3PlayerUp * 5));


#endif

	Camera::Update();
}
