#pragma once
#include "Camera.h"

class ThirdPersonCamera : public Camera {
public:
	ThirdPersonCamera();
	~ThirdPersonCamera();

	virtual void ProcessInput() override;
	virtual void Update() override;

	void SetOffset(const Vector3& v3Offset) { m_v3Offset = v3Offset; }
	void SetTimeLag(float fTimeLag) { m_fTimeLag = fTimeLag; }

private:
	Vector3 m_v3Offset;
	float m_fTimeLag = 0.f;
	const float m_fMouseSensitivity = 0.001f;
	float m_fPlayerSpeed = 50.f;

	Vector3 m_v3CameraLookCache;
};

