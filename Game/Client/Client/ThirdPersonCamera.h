#pragma once
#include "Camera.h"

enum class CAMERA_MODE {
	FREE,
	AIM
};

class ThirdPersonCamera : public Camera {
public:
	ThirdPersonCamera();

	virtual void Update() override;

public:
	void EnterAimMode();
	void LeaveAimMode();

public:
	void AddYaw(float fValue);
	void AddPitch(float fValue);

	void SetOffset(const Vector3& v3Value);
	
	// 바라볼 위치 (대충 가슴높이) 조절 : Y 높이
	// 조준 모드에 들어가면 이 값을 조절함
	void SetLookHeight(float fValue);

	Vector3 GetForwardXZ() const;
	Vector3 GetRightXZ() const;



private:
	void UpdateFreeMode();
	void UpdateAimMode();

private:
	CAMERA_MODE m_eCameraMode;

	Vector3 m_v3Offset;
	Vector3 m_v3AimOffset;

	float m_fLookHeight;

	float m_fMinPitch = -70.f;
	float m_fMaxPitch = 30.f;

	const Vector3 m_v3FreeModeOffset = Vector3{ 0, 1.5_m, -2.0_m };
	const Vector3 m_v3AimModeOffset = Vector3{ 30_cm, 1.5_m, -80_cm };

	const float m_v3FreeModeHeight = 1.2_m;
	const float m_v3AimModeHeight = 1.55_m;

};

