#pragma once
#include "DynamicObject.h"

class WeaponObject : public DynamicObject {
public:
	WeaponObject() {}
	virtual ~WeaponObject() {}

	void Initialize() override;
	void ProcessInput()  override {};
	void Update() override;

	bool TryFire();
	void UpdateMuzzlePositionWorld(const Matrix& mtxWorld);
	
	void SetWeaponType(WEAPON_TYPE eWeaponType) { m_eWeaponType = eWeaponType; }
	WEAPON_TYPE GetWeaponType() const { return m_eWeaponType; }

	void SetDamage(float fValue) { m_fDamage = fValue; }
	void SetFirePerSecond(float fValue) { m_fFirePerSecond = fValue; m_fFireInterval = 60.f / fValue; }
	void SetRecoil(float fValue) { m_fRecoil = fValue; }
	void SetRecoilRecovery(float fValue) { m_fRecoilRecovery = fValue; }
	void SetReloadTime(float fValue) { m_fReloadTime = fValue; }
	void SetOffsetPosition(const Vector3& v3Pos) { m_v3OffsetPosition = v3Pos; }
	void SetOffsetRotation(const Vector3& v3Rotation) { m_v3OffsetRotation = v3Rotation; }
	void SetMuzzlePositionLocal(const Vector3& v3Pos) { m_v3MuzzlePositionLocal = v3Pos; }

	float GetDamage() const { return m_fDamage; }
	float GetFirePerSecond() const { return m_fFirePerSecond; }
	float GetRecoil() const { return m_fRecoil; }
	float GetRecoilRecovery() const { return m_fRecoilRecovery; }
	float GetReloadTime() const { return m_fReloadTime; }
	const Vector3& GetOffsetPosition() const { return m_v3OffsetPosition; }
	const Vector3& GetOffsetRotation() const { return m_v3OffsetRotation; }
	const Vector3& GetMuzzlePositionLocal(const Matrix& mtxWorld) const { return m_v3MuzzlePositionLocal; }
	const Vector3& GetMuzzlePositionWorld(const Matrix& mtxWorld) const { return m_v3MuzzlePositionWorld; }

	const Matrix& GetOffsetTransform();

public:
	void EditStat();
	void SaveStat();

private:
	float m_fDamage = 0.f;
	float m_fFirePerSecond = 0.f;
	float m_fRecoil = 0.f;
	float m_fRecoilRecovery = 0.f;
	float m_fReloadTime = 0.f;

	float m_fFireInterval = 0.f;
	float m_fTimeAfterFire = 0.f;

	Vector3 m_v3MuzzlePositionLocal = Vector3::Zero;
	Vector3 m_v3MuzzlePositionWorld = Vector3::Zero;

	Vector3 m_v3OffsetPosition = Vector3::Zero;
	Vector3 m_v3OffsetRotation = Vector3::Zero;
	Matrix m_mtxOffsetTransform = Matrix::Identity;

	WEAPON_TYPE m_eWeaponType = WEAPON_TYPE::UNDEFINED;
};

