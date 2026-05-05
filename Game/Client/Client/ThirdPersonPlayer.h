#pragma once
#include "Player.h"
#include "BoundingCapsule.h"

/*
	- 구조
	ThirdPersonPlayer - 공통 몸체, 애니메이션, 무기, 충돌, 서버 상태/이벤트 적용
	 ├─ LocalThirdPersonPlayer - 기존 싱글/로컬 방식
	 ├─ NetworkOwnerThirdPersonPlayer
	 │   └─ 입력 읽음, 서버에 명령 전달, 서버 상태/이벤트 받아 반영
	 └─ NetworkRemoteThirdPersonPlayer
		 └─ 입력 없음, 서버 상태/이벤트 받아 반영

	- 목표
	클래스							입력 읽기	직접 이동	서버 XZ 수신	클라 Y 결정		애니메이션				카메라	크로스헤어
	LocalThirdPersonPlayer			    O			O			 X				O			로컬 입력으로 직접 재생	   O		O
	NetworkOwnerThirdPersonPlayer	    O			X			 O				O			서버 상태/이벤트로 재생	   O		O
	NetworkRemoteThirdPersonPlayer	    X			X			 O				O			서버 상태/이벤트로 재생	   X		X
*/

class IThirdPersonPlayer : public IPlayer {
public:
	IThirdPersonPlayer();
	virtual ~IThirdPersonPlayer();

public:
	void Initialize() override;
	virtual void ProcessInput() override = 0;
	virtual void Update() override;
	virtual void PostUpdate() override;

public:
	// Getter
	float GetMoveSpeed() const { return m_fMoveSpeed; }
	float GetMoveSpeedXZ() const;
	float GetMoveSpeedSqXZ() const;
	const Vector3& GetMoveDirection() const { return m_v3MoveDirection; }

	bool IsAiming() const { return m_bAiming; }
	bool IsRunning() const { return m_bRunning; }
	bool IsMouseOn() const { return m_bMouseInUse; }
	bool ConsumeFire() { bool b = m_bFiredThisFrame; m_bFiredThisFrame = false; return b; }

public:
	// Collision
	virtual void OnBeginCollision(const CollisionResult& collisionResult) override;
	virtual void OnWhileCollision(const CollisionResult& collisionResult) override;
	virtual void OnEndCollision(const CollisionResult& collisionResult) override;

public:
	// Player modifier
	void GiveWeapon(WEAPON_TYPE eWeaponType);

public:
	// Callbacks
	void OnMeleeEnd();

public:
	// Packet handling

	// "플레이어 지속 상태" 패킷을 반영
	// 위치, 회전, HP, 조준/달리기/무기상태 등의 지속적인 상태를 처리
	virtual void ApplyReplicatedState(/* const ServerSidePlayerState& state */);

	// "플레이어 이벤트" 패킷을 반영
	// 발사, 근접공격, 피격, 사망, 무기변경 등의 1회성 동작을 처리
	virtual void ApplyReplicatedEvent(/* const ServerSidePlayerEvent& event */);

protected:
	virtual void SendLocalCommandToServer() {}

protected:
	// Derived player attributes
	virtual bool UsesLocalCamera() const { return false; }
	virtual bool UsesCrosshair() const { return false; }
	virtual bool UsesInputMovement() const { return false; }
	virtual bool UsesServerStateMovement() const { return false; }
	virtual bool NeedsSendMovementState() const { return false; }

protected:
	// Initializer
	void InitializeCommonPlayer();
	void InitializeLocalCamera();
	void InitializeCrosshair();

protected:
	// PrecessInput() roll seperated
	void ProcessLocalCameraInput();
	void ProcessLocalMovementInput();
	void ProcessLocalActionInput();

protected:
	// Player Move
	void ApplyInputMovement();					// 기존의 HandleCollision() 의 기능이 이동됨 : 이동 적용 + 충돌 처리
	void ResolveGroundYOnly(Vector3& delta);	// 서버가 XZ 좌표만 넘겨줄 때 클라에서는 Y만 결정하
	void SetServerTargetXZ(float x, float z);
	void ApplyServerMovementXZ();

protected:
	// Player action
	void EnterAim();
	void LeaveAim();

	void PlayFireAction();
	void PlayMeleeStartAction();
	void PlayMeleeEndAction();

	void ApplyWeaponChanged(WEAPON_TYPE eWeaponType);
	void ApplyHitReact(float damage);
	void ApplyDead();

protected:
	// Ground solving
	void ApplyGravity();
	void ResolveCollision(OUT Vector3& outv3Delta);
	bool CheckGround(float fMaxDistance, OUT Vector3& outv3Normal);
	bool TryStepUp(
		const BoundingCapsule& capsule,
		const BoundingOrientedBox& box,
		OUT Vector3& outv3Delta);

protected:
	// Mouse options
	void ToggleMouseLook();
	void OnBeginMouseLook();
	void OnEndMouseLook();
	void UpdateMouseLookData();

private:
	// Leave it for merge
	void HandleCollision();

protected:
	// Move
	Vector3	m_v3MoveDirection;
	float	m_fMoveSpeed = 0.f;
	float	m_fVerticalVelocity = 0.f;

	// Server move
	Vector2 m_v2ServerTargetXZ = Vector2::Zero;
	bool m_bHasServerTargetXZ = false;

	// State flags
	bool m_bMoved = false;
	bool m_bAiming = false;
	bool m_bRunning = false;
	bool m_bFiredThisFrame = false;

	WEAPON_TYPE m_eWeaponTypeBeforeMelee = WEAPON_TYPE::UNDEFINED;
	bool m_bInMeleeAttack = false;
	bool m_bWasAimBeforeMelee = false;

	// Collision
	std::vector<BoundingOrientedBox> m_xmOBBCollided;

	// Player Components
	std::shared_ptr<class Crosshair> m_pCrosshair = nullptr;
	std::shared_ptr<WeaponSocket> m_pWeaponSocket = nullptr;

protected:
	// Movement constants
	const float	m_fMaxMoveSpeed = 1.4_m;
	const float	m_fAcceleration = 10.0_cm;
	const float	m_fFriction = 10.f;
	const float	m_fGravity = -9.8_cm * 10;

	// Ground check constants
	uint32			m_unGroundGraceFrames = 0;
	const uint32	m_unMaxGroundGraceFrames = 4;
	const float		m_fGroundDeadZoneY = 0.02_cm;
	const float		m_fStepHeight = 50_cm;

	// Mouse input
	const float	m_fMouseSensitivity = 0.1f;
	POINT m_ptMouseCenterClientPos = {};
	POINT m_ptMouseCenterScreenPos = {};
	RECT  m_MouseClipScreenRect = {};

	// Debug value
	bool m_bMouseInUse = false;
	bool m_bSkipMouseDeltaThisFrame = false;	// To prevent twitch when mouse look enabled

};

class LocalThirdPersonPlayer final : public IThirdPersonPlayer {
public:
	void ProcessInput() override;

protected:
	bool UsesLocalCamera() const override { return true; }
	bool UsesCrosshair() const override { return true; }
	bool UsesInputMovement() const override { return true; }
	bool UsesServerStateMovement() const override { return false; }
	bool NeedsSendMovementState() const override { return false; }
};

class NetworkOwnerThirdPersonPlayer final : public IThirdPersonPlayer {
public:
	void ProcessInput() override;

protected:
	bool UsesLocalCamera() const override { return true; }
	bool UsesCrosshair() const override { return true; }
	bool UsesInputMovement() const override { return true; }
	bool UsesServerStateMovement() const override { return true; }
	bool NeedsSendMovementState() const override { return true; }

protected:
	virtual void SendLocalCommandToServer() override;
};

class NetworkRemoteThirdPersonPlayer final : public IThirdPersonPlayer {
public:
	void ProcessInput() override {}	// 입력 처리 받지 않음

protected:
	bool UsesLocalCamera() const override { return false; }
	bool UsesCrosshair() const override { return false; }
	bool UsesInputMovement() const override { return false; }
	bool UsesServerStateMovement() const override { return true; }
	bool NeedsSendMovementState() const override { return false; }
};
