#include "pch.h"
#include "ThirdPersonCamera.h"

//#define CAMERA_WITH_DELAY

ThirdPersonCamera::ThirdPersonCamera()
{
	m_eCameraMode = CAMERA_MODE::FREE;
	m_v3Offset = m_v3FreeModeOffset;
	m_fLookHeight = m_v3FreeModeHeight;
}

void ThirdPersonCamera::Update()
{
	if (m_wpOwner.expired()) {
		return;
	}

	switch (m_eCameraMode)
	{
	case CAMERA_MODE::FREE:
	{
		UpdateFreeMode();
		break;
	}
	case CAMERA_MODE::AIM:
	{
		UpdateAimMode();
		break;
	}
	}

	Camera::Update();
}

void ThirdPersonCamera::UpdateFreeMode()
{
	const auto& ownerTransform = m_wpOwner.lock()->GetTransform();
	Vector3 v3TargetPos = ownerTransform.GetPosition();

	Matrix mtxRotation = Matrix::CreateFromYawPitchRoll(
		XMConvertToRadians(m_fYaw),
		XMConvertToRadians(m_fPitch),
		0.f
	);

	Vector3 v3WorldOffset = Vector3::TransformNormal(m_v3Offset, mtxRotation);
	Vector3 v3CameraPos = v3TargetPos + v3WorldOffset;

	SetPosition(v3CameraPos);

	Vector3 v3LookTarget = v3TargetPos;
	v3LookTarget.y += m_fLookHeight;
	SetLookAt(v3LookTarget);
}

void ThirdPersonCamera::UpdateAimMode()
{
	const auto& ownerTransform = m_wpOwner.lock()->GetTransform();
	Vector3 v3TargetPos = ownerTransform.GetPosition();

	// 카메라 위치
	Matrix mtxYawRotation = Matrix::CreateRotationY(XMConvertToRadians(m_fYaw));
	Vector3 v3ShoulderOffset = Vector3::TransformNormal(m_v3Offset, mtxYawRotation);
	Vector3 v3CameraPos = v3TargetPos + v3ShoulderOffset;
	SetPosition(v3CameraPos);

	// 시선 방향
	Matrix mtxLookRotation = Matrix::CreateFromYawPitchRoll(
		XMConvertToRadians(m_fYaw),
		XMConvertToRadians(m_fPitch),
		0.f
	);

	Vector3 v3LookDir = Vector3::TransformNormal(Vector3::Backward, mtxLookRotation);
	SetLookAt(v3CameraPos + (v3LookDir * 1000.f));
}

void ThirdPersonCamera::EnterAimMode()
{
	m_eCameraMode = CAMERA_MODE::AIM;
	m_v3Offset = m_v3AimModeOffset;
	m_fLookHeight = m_v3AimModeHeight;
}

void ThirdPersonCamera::LeaveAimMode()
{
	m_eCameraMode = CAMERA_MODE::FREE;
	m_v3Offset = m_v3FreeModeOffset;
	m_fLookHeight = m_v3FreeModeHeight;
}

void ThirdPersonCamera::AddYaw(float fValue)
{
	m_fYaw += fValue;
}

void ThirdPersonCamera::AddPitch(float fValue)
{
	m_fPitch += fValue;
	m_fPitch = std::clamp(m_fPitch, m_fMinPitch, m_fMaxPitch);
}

void ThirdPersonCamera::SetOffset(const Vector3& v3Value)
{
	m_v3Offset = v3Value;
}

void ThirdPersonCamera::SetLookHeight(float fValue)
{
	m_fLookHeight = fValue;
}

Vector3 ThirdPersonCamera::GetForwardXZ() const
{
	Vector3 v3LookXZ = m_v3Look;
	v3LookXZ.y = 0.f;
	v3LookXZ.Normalize();

	return v3LookXZ;
}

Vector3 ThirdPersonCamera::GetRightXZ() const
{
	Vector3 v3RightXZ = m_v3Right;
	v3RightXZ.y = 0.f;
	v3RightXZ.Normalize();

	return v3RightXZ;
}
