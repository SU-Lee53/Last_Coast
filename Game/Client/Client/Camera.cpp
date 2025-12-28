#include "pch.h"
#include "Camera.h"


Camera::Camera()
{
	m_mtxView = Matrix::Identity;
	m_mtxProjection = Matrix::Identity;
	//m_mtxCameraTransform = Matrix::Identity;
}

Camera::~Camera()
{
}

void Camera::Update()
{
	GenerateViewMatrix();
}

Vector3 Camera::GetPosition() const
{
	return m_v3Position;
}

Vector3 Camera::GetRight() const
{
	return m_v3Right;
}

Vector3 Camera::GetUp() const
{
	return m_v3Up;
}

Vector3 Camera::GetLook() const
{
	return m_v3Look;
}

float Camera::GetPitch() const
{
	return m_fPitch;
}

float Camera::GetYaw() const
{
	return m_fYaw;
}

float Camera::GetRoll() const
{
	return m_fRoll;
}

void Camera::SetPosition(float x, float y, float z)
{
	SetPosition(Vector3(x, y, z));
}

void Camera::SetPosition(const Vector3& v3Position)
{
	m_v3Position = v3Position;
}

void Camera::SetRotation(float fPitch, float fYaw, float fRoll)
{
	m_fPitch = fPitch;
	m_fYaw = fYaw;
	m_fRoll = fRoll;

	Matrix mtxRotate = Matrix::CreateFromYawPitchRoll(m_fYaw, m_fPitch, m_fRoll);
	m_v3Right = Vector3::TransformNormal(Vector3::Right, mtxRotate);
	m_v3Up = Vector3::TransformNormal(Vector3::Up, mtxRotate);
	m_v3Look = Vector3::TransformNormal(Vector3::Backward, mtxRotate);

	m_v3Right.Normalize();
	m_v3Up.Normalize();
	m_v3Look.Normalize();
}

void Camera::SetLookAt(float x, float y, float z)
{
	SetLookAt(Vector3(x, y, z));
}

void Camera::SetLookAt(const Vector3& v3Look)
{
	Matrix mtxLookAt = XMMatrixLookAtLH(m_v3Position, v3Look, m_wpOwner.lock()->GetTransform().GetUp());
	m_v3Right = Vector3(mtxLookAt._11, mtxLookAt._21, mtxLookAt._31);
	m_v3Up = Vector3(mtxLookAt._12, mtxLookAt._22, mtxLookAt._32);
	m_v3Look = Vector3(mtxLookAt._13, mtxLookAt._23, mtxLookAt._33);
}

void Camera::Rotate(float fPitch, float fYaw, float fRoll)
{
}

void Camera::Rotate(const Vector3 v3Rotation)
{
}

void Camera::GenerateViewMatrix()
{
	m_v3Look.Normalize();
	m_v3Right = XMVector3Cross(m_v3Up, m_v3Look);
	m_v3Right.Normalize();
	m_v3Up = XMVector3Cross(m_v3Look, m_v3Right);
	m_v3Up.Normalize();

	m_mtxView._11 = m_v3Right.x; m_mtxView._12 = m_v3Up.x; m_mtxView._13 = m_v3Look.x;
	m_mtxView._21 = m_v3Right.y; m_mtxView._22 = m_v3Up.y; m_mtxView._23 = m_v3Look.y;
	m_mtxView._31 = m_v3Right.z; m_mtxView._32 = m_v3Up.z; m_mtxView._33 = m_v3Look.z;
	m_mtxView._41 = -m_v3Position.Dot(m_v3Right);
	m_mtxView._42 = -m_v3Position.Dot(m_v3Up);
	m_mtxView._43 = -m_v3Position.Dot(m_v3Look);
}

void Camera::GenerateViewMatrix(Vector3 v3Position, Vector3 v3LookAt, Vector3 v3Up)
{
	m_v3Position = v3Position;
	m_v3Look = v3LookAt;
	m_v3Up = v3Up;

	GenerateViewMatrix();
}

void Camera::GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle)
{
	m_ffovY = fFOVAngle;
	m_fAspectRatio = fAspectRatio;
	m_fNear = fNearPlaneDistance;
	m_fFar = fFarPlaneDistance;
	
	float FOVAngleInRad = XMConvertToRadians(m_ffovY);

	XMStoreFloat4x4(&m_mtxProjection,
		XMMatrixPerspectiveFovLH(
			FOVAngleInRad,
			fAspectRatio,
			fNearPlaneDistance,
			fFarPlaneDistance
		)
	);

	BoundingFrustum::CreateFromMatrix(m_xmFrustum, XMLoadFloat4x4(&m_mtxProjection));
}

void Camera::SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ, float fMaxZ)
{
	m_d3dViewport.TopLeftX = xTopLeft;
	m_d3dViewport.TopLeftY = yTopLeft;
	m_d3dViewport.Width = nWidth;
	m_d3dViewport.Height = nHeight;
	m_d3dViewport.MinDepth = fMinZ;
	m_d3dViewport.MaxDepth = fMaxZ;
}

void Camera::SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom)
{
	m_d3dScissorRect.left = xLeft;
	m_d3dScissorRect.right = xRight;
	m_d3dScissorRect.top = yTop;
	m_d3dScissorRect.bottom = yBottom;
}

void Camera::SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
}

CB_CAMERA_DATA Camera::MakeCBData() const
{
	CB_CAMERA_DATA camData{
		.mtxView = m_mtxView.Transpose(),
		.mtxProjection = m_mtxProjection.Transpose(),
		.v3CameraPosition = m_v3Position
	};


	return camData;
}

Matrix Camera::GetViewProjectMatrix() const
{
	Matrix ret;

	XMMATRIX xmmtxView = XMLoadFloat4x4(&m_mtxView);
	XMMATRIX xmmtxProject = XMLoadFloat4x4(&m_mtxProjection);

	XMStoreFloat4x4(&ret, XMMatrixMultiply(xmmtxView, xmmtxProject));
	return ret;
}

Matrix Camera::GetProjectionMatrix() const
{
	return m_mtxProjection;
}

Matrix Camera::GetViewMatrix() const
{
	return m_mtxView;
}

float Camera::GetNearPlaneDistance() const
{
	return m_fNear;
}

float Camera::GetFarPlaneDistance() const
{
	return m_fFar;
}
