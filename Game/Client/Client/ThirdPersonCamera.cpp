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
}

void ThirdPersonCamera::Update()
{
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

	Camera::Update();
}
