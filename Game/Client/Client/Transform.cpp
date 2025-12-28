#include "pch.h"
#include "Transform.h"
#include "GameObject.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	- 주의
//		- SimpleMath 가 RHS 좌표계 사용중이라 Forward(Look) 방향은 Matrix::Backward() 로 가져와야 함
//

Transform::Transform()
{
	m_mtxTransform = Matrix::Identity;
	m_mtxWorld = Matrix::Identity;
}

void Transform::Update(std::shared_ptr<GameObject> pParent)
{
	// 11.15 수정
	// 외부 수정 flag 를 내리고 업데이트 없이 리턴함
	if (m_bWorldSetFromOutside) {
		m_bWorldSetFromOutside = false;
		return;
	}
	m_mtxWorld = (pParent) ? ((m_mtxTransform * m_mtxFrameRelative) * pParent->GetTransform().m_mtxWorld) : (m_mtxTransform * m_mtxFrameRelative);
}

void Transform::SetFrameMatrix(const Matrix& mtxFrame)
{
	m_mtxFrameRelative = mtxFrame;
}

void Transform::SetWorldMatrix(const Matrix& mtxWorld)
{
	m_mtxWorld = mtxWorld;
	m_bWorldSetFromOutside = true;
}

//void Transform::SetLocalMatrix(const Matrix& xmf4x4Local)
//{
//	m_mtxTransform = xmf4x4Local;
//}

// =========
// Translate
// =========

void Transform::SetPosition(float x, float y, float z)
{
	m_mtxTransform.Translation(Vector3(x, y, z));
}

void Transform::SetPosition(const Vector3& v3Position)
{
	m_mtxTransform.Translation(v3Position);
}

void Transform::Move(Vector3 v3MoveDirection, float fAmount)
{
	Vector3 v3CurrentPos = m_mtxTransform.Translation();
	m_mtxTransform.Translation(v3CurrentPos + (v3MoveDirection * fAmount));
}

// ======
// Rotate
// ======

void Transform::SetRotation(float fPitch, float fYaw, float fRoll)
{
	Matrix mtxRotation = Matrix::CreateFromYawPitchRoll(fYaw, fPitch, fRoll);
	mtxRotation = mtxRotation * Matrix::CreateTranslation(m_mtxTransform.Translation());
	m_mtxTransform = mtxRotation;
}

void Transform::SetRotation(const Vector3& xmf3Rotation)
{
	Matrix mtxRotation = Matrix::CreateFromYawPitchRoll(xmf3Rotation);
	mtxRotation = mtxRotation * Matrix::CreateTranslation(m_mtxTransform.Translation());
	m_mtxTransform = mtxRotation;
}

void Transform::Rotate(const Vector3& v3Rotation)
{
	m_mtxTransform = Matrix::CreateFromYawPitchRoll(v3Rotation) * m_mtxTransform;
}

void Transform::Rotate(float fPitch, float fYaw, float fRoll)
{
	m_mtxTransform = Matrix::CreateFromYawPitchRoll(XMConvertToRadians(fYaw), XMConvertToRadians(fPitch), XMConvertToRadians(fRoll)) * m_mtxTransform;
}

void Transform::Rotate(const Vector3& v3Axis, float fAngle)
{
	m_mtxTransform = Matrix::CreateFromAxisAngle(v3Axis, XMConvertToRadians(fAngle)) * m_mtxTransform;
}

void Transform::RotateWorld(float fPitch, float fYaw, float fRoll, const Vector3& v3BaseOnWorld)
{
	m_mtxTransform = m_mtxTransform * Matrix::CreateFromYawPitchRoll(XMConvertToRadians(fYaw), XMConvertToRadians(fPitch), XMConvertToRadians(fRoll)) * Matrix::CreateTranslation(v3BaseOnWorld);
}

void Transform::RotateWorld(const Vector3& v3Axis, float fAngle, const Vector3& v3BaseOnWorld)
{
	m_mtxTransform = m_mtxTransform * Matrix::CreateTranslation(-v3BaseOnWorld) * Matrix::CreateFromAxisAngle(v3Axis, XMConvertToRadians(fAngle)) * Matrix::CreateTranslation(v3BaseOnWorld);
}

void Transform::Scale(float fxScale, float fyScale, float fzScale)
{
	m_mtxTransform = Matrix::CreateScale(fxScale, fyScale, fzScale) * m_mtxTransform;
}

void Transform::Scale(const Vector3& v3Scale)
{
	m_mtxTransform = Matrix::CreateScale(v3Scale) * m_mtxTransform;
}

void Transform::Scale(float fScale)
{
	m_mtxTransform = Matrix::CreateScale(fScale) * m_mtxTransform;
}

// =====
// Basis
// =====

Vector3 Transform::GetPosition() const
{
	return m_mtxWorld.Translation();
}

Vector3 Transform::GetRight() const
{
	return m_mtxWorld.Right();
}

Vector3 Transform::GetUp() const
{
	return m_mtxWorld.Up();
}

Vector3 Transform::GetLook() const
{
	return m_mtxWorld.Backward();
}

Matrix Transform::GetWorldMatrix() const
{
	return m_mtxWorld;
}
