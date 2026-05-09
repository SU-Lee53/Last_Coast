#pragma once
#include "DynamicObject.h"
#include "ThirdPersonPlayer.h"

class HitMarker;

class WeaponObject : public DynamicObject {
public:
	WeaponObject() {}
	virtual ~WeaponObject() {}

	void Initialize() override;
	void ProcessInput()  override {};
	void Update() override;

	bool TryFire();
	bool BeginReload();
	bool EndReload();
	void UpdateMuzzlePositionWorld(const Matrix& mtxWorld);

	void SetWeaponType(WEAPON_TYPE eWeaponType) { m_eWeaponType = eWeaponType; }
	WEAPON_TYPE GetWeaponType() const { return m_eWeaponType; }

	void SetDamage(float fValue) { m_fDamage = fValue; }
	void SetFirePerSecond(float fValue) { m_fFirePerSecond = fValue; m_fFireInterval = 60.f / fValue; }
	void SetRecoil(float fValue) { m_fRecoil = fValue; }
	void SetRecoilRecovery(float fValue) { m_fRecoilRecovery = fValue; }
	void SetAmmoPerClip(int32 nValue) { m_nAmmoPerClip = nValue; }
	void SetCurrentAmmoInClip(int32 nValue) { m_nAmmoInClip = nValue; }
	void SetOffsetPosition(const Vector3& v3Pos) { m_v3OffsetPosition = v3Pos; }
	void SetOffsetRotation(const Vector3& v3Rotation) { m_v3OffsetRotation = v3Rotation; }
	void SetMuzzlePositionLocal(const Vector3& v3Pos) { m_v3MuzzlePositionLocal = v3Pos; }
	void SetOwner(const std::shared_ptr<IThirdPersonPlayer>& pOwnerPlayer) { m_wpOwner = pOwnerPlayer; }

	float GetDamage() const { return m_fDamage; }
	float GetFirePerSecond() const { return m_fFirePerSecond; }
	float GetRecoil() const { return m_fRecoil; }
	float GetRecoilRecovery() const { return m_fRecoilRecovery; }
	int32 GetAmmoPerClip() const { return m_nAmmoPerClip; }
	const Vector3& GetOffsetPosition() const { return m_v3OffsetPosition; }
	const Vector3& GetOffsetRotation() const { return m_v3OffsetRotation; }
	const Vector3& GetMuzzlePositionLocal(const Matrix& mtxWorld) const { return m_v3MuzzlePositionLocal; }
	const Vector3& GetMuzzlePositionWorld(const Matrix& mtxWorld) const { return m_v3MuzzlePositionWorld; }
	int32 GetTotalAmmo() const { return m_nTotalAmmo; };
	int32 GetAmmoInClip() const { return m_nAmmoInClip; };
	bool IsInReloading() const { return m_bInReload; }
	
	std::shared_ptr<IThirdPersonPlayer> GetOwner() { return m_wpOwner.lock(); }

	const Matrix& GetOffsetTransform();

	RayTraceDesc GenerateRayTraceDesc();

public:
	void EditStat();
	void SaveStat();

private:
	float m_fDamage = 0.f;
	float m_fFirePerSecond = 0.f;
	float m_fRecoil = 0.f;
	float m_fRecoilRecovery = 0.f;
	//float m_fReloadTime = 0.f;

	float m_fFireInterval = 0.f;
	float m_fTimeAfterFire = 0.f;

	Vector3 m_v3MuzzlePositionLocal = Vector3::Zero;
	Vector3 m_v3MuzzlePositionWorld = Vector3::Zero;

	Vector3 m_v3OffsetPosition = Vector3::Zero;
	Vector3 m_v3OffsetRotation = Vector3::Zero;
	Matrix m_mtxOffsetTransform = Matrix::Identity;

	int32 m_nAmmoPerClip = 999;
	int32 m_nTotalAmmo = 999;
	int32 m_nAmmoInClip = 0;

	float m_fReloadTimer = 0.f;
	bool m_bInReload = false;
	

	WEAPON_TYPE m_eWeaponType = WEAPON_TYPE::UNDEFINED;

	std::weak_ptr<IThirdPersonPlayer> m_wpOwner;
};

