#pragma once



class Camera {
public:
	Camera();
	~Camera();

	virtual void ProcessInput() {}
	virtual void Update();

public:
	void SetOwner(std::shared_ptr<IGameObject> pOwner) { m_wpOwner = pOwner; }

public:
	Matrix GetViewProjectMatrix() const;

	const Matrix& GetProjectionMatrix() const;
	const Matrix& GetViewMatrix() const;
	const Matrix& GetCameraWorldTransfromMatrix() const;

	float GetNearPlaneDistance() const;
	float GetFarPlaneDistance() const;

	const Vector3& GetPosition() const;
	const Vector3& GetRight() const;
	const Vector3& GetUp() const;
	const Vector3& GetLook() const;
	
	float GetPitch() const;
	float GetYaw() const;
	float GetRoll() const;
	
	float GetFovYInRadian() const;
	float GetAspectRatio() const;

	const BoundingFrustum& GetFrustumOrigin() const { return m_xmFrustumOrigin; }
	const BoundingFrustum& GetFrustumWorld() const { return m_xmFrustumWorld; }

	const Vector4& GetCascadeSplits() const { return m_v4CascadeSplits; }

public:
	void SetPosition(float x, float y, float z);
	void SetPosition(const Vector3& v3Position);
	void SetRotation(float fPitch, float fYaw, float fRoll);
	void SetLookAt(float x, float y, float z);
	void SetLookAt(const Vector3& v3Look);
	void SetLookTo(float x, float y, float z);
	void SetLookTo(const Vector3& v3Look);

	void Rotate(float x, float y, float z);
	void Rotate(const Vector3 v3Rotation);

public:
	void GenerateViewMatrix();
	void GenerateViewMatrix(Vector3 v3Position, Vector3 v3LookAt, Vector3 v3Up);

	void GenerateProjectionMatrix(float fNearPlaneDistance, float fFarPlaneDistance, float fAspectRatio, float fFOVAngle);

	void SetViewport(int xTopLeft, int yTopLeft, int nWidth, int nHeight, float fMinZ = 0.0f, float fMaxZ = 1.0f);
	void SetScissorRect(LONG xLeft, LONG yTop, LONG xRight, LONG yBottom);

	virtual void SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	CameraData MakeCBData() const;

private:
	void ComputeCascadeSplits();

protected:
	BoundingFrustum m_xmFrustumOrigin = {};
	BoundingFrustum m_xmFrustumWorld = {};

	Matrix m_mtxWorld;
	Matrix m_mtxView;
	Matrix m_mtxProjection;

	Matrix m_mtxInverseView;
	Matrix m_mtxInverseProjection;

	float m_ffovY = 0.f;
	float m_fAspectRatio = 0.f;
	float m_fNear = 0.f;
	float m_fFar = 0.f;

	Vector3 m_v3Position;
	Vector3 m_v3Right;
	Vector3 m_v3Up;
	Vector3 m_v3Look;

	float	m_fPitch = 0.f;
	float	m_fRoll = 0.f;
	float	m_fYaw = 0.f;

	Vector4 m_v4CascadeSplits;

	D3D12_VIEWPORT	m_d3dViewport = {};
	D3D12_RECT		m_d3dScissorRect = {};

	ComPtr<ID3D12GraphicsCommandList> m_pd3dViewportScissorrectBindingBundleCommand = nullptr;

	std::weak_ptr<IGameObject> m_wpOwner;

};

