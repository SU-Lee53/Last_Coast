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

class Crosshair;
class HitMarker;
class TextBox;


class IThirdPersonPlayer : public IPlayer {
private:
	struct PlayerHUD {
		std::shared_ptr<Crosshair>	pCrosshair = nullptr;
		std::shared_ptr<HitMarker>	pHitMarker = nullptr;
		std::shared_ptr<TextBox>	pHealthText = nullptr;
		std::shared_ptr<TextBox>	pHealthImage = nullptr;
		std::shared_ptr<TextBox>	pAmmo = nullptr;
		std::shared_ptr<TextBox>	pWeaponName = nullptr;
		std::shared_ptr<TextBox>	pWeaponSlots = nullptr;	// 1/2/3 슬롯 목록 + 현재 슬롯 하이라이트
		std::shared_ptr<TextBox>	pReloadAlert = nullptr;

		void Initialize(const IThirdPersonPlayer& player);
		void Update(const IThirdPersonPlayer& player);
	};


public:
	IThirdPersonPlayer();
	virtual ~IThirdPersonPlayer();

public:
	void Initialize() override;
	virtual void ProcessInput() override = 0;
	virtual void Update() override;
	virtual void PostUpdate() override;
	virtual void AddToQueue(OUT std::vector<IGameObject*>& pRenderQueue) override;

public:
	// Getter
	float GetMoveSpeed() const { return m_fMoveSpeed; }
	float GetMoveSpeedXZ() const;
	float GetMoveSpeedSqXZ() const;
	const Vector3& GetMoveDirection() const { return m_v3MoveDirection; }
	std::shared_ptr<WeaponObject> GetCurrentWeaponObject() const { return m_pWeaponSocket->GetWeaponModel(); }
	WEAPON_TYPE GetCurrentWeaponType() const { return m_pWeaponSocket->GetCurrentWeaponType(); }

	bool IsAiming() const { return m_bAiming; }
	float GetAimPitch() const { return m_fAimPitch; }
	bool IsMoving() const { return m_bMoved; }
	bool IsRunning() const { return m_bRunning; }
	bool IsMouseOn() const { return m_bMouseInUse; }
	bool ConsumeFire() { bool b = m_bFiredThisFrame; m_bFiredThisFrame = false; return b; }
	bool ConsumeMelee() { bool b = m_bMeleeStartedThisFrame; m_bMeleeStartedThisFrame = false; return b; }

	void SetPlayerModel(const std::string& strModelKey);

public:
	// Collision
	virtual void OnBeginCollision(const CollisionResult& collisionResult) override;
	virtual void OnWhileCollision(const CollisionResult& collisionResult) override;
	virtual void OnEndCollision(const CollisionResult& collisionResult) override;

public:
	// Player modifier
	void GiveWeapon(WEAPON_TYPE eWeaponType);

	// 무기 슬롯 (로컬 플레이어 전용): 0/1번 = 로비에서 고른 주무기, 2번 = 권총 고정.
	// 슬롯별 WeaponObject 인스턴스를 캐시해 전환해도 탄약이 유지된다.
	void SetWeaponSlots(WEAPON_TYPE eWeapon1, WEAPON_TYPE eWeapon2);
	void SelectWeaponSlot(int nSlot);
	int GetCurrentWeaponSlot() const { return m_nCurrentWeaponSlot; }
	WEAPON_TYPE GetWeaponInSlot(int nSlot) const { return m_eWeaponSlots[nSlot]; }
	bool HasWeaponSlots() const { return m_pSlotWeapons[0] != nullptr; }

public:
	// Callbacks
	void OnMeleeEnd();
	void OnReloadEnd();
	void OnWeaponDrawEnd();		// "Weapon Draw" 몽타주 종료 notify — 교체 잠금 해제
	void ShowHitMarker();

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
	virtual bool UsesHUD() const { return false; }
	virtual bool UsesInputMovement() const { return false; }
	virtual bool UsesServerStateMovement() const { return false; }
	virtual bool NeedsSendMovementState() const { return false; }

protected:
	// Initializer
	void InitializeCommonPlayer();
	void InitializeLocalCamera();

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
public:
	void PlayReloadStartAction();
	void PlayReloadEndAction();

public:
	void PlayFireAction();
	void PlayMeleeStartAction();	// public: 리모트 플레이어 근접공격 모션 재생에도 사용
	void PlayWeaponDrawAction();	// public: 리모트 플레이어 무기교체 모션 재생에도 사용
	void LeaveAim();				// public: 컷씬 진입 시 Scene::PushCinematic이 조준 해제에 사용 (멱등 — 비조준이면 no-op)

	// 입력 처리가 차단된 프레임(컷씬/로딩 대기)에 호출 — 직전 프레임의 이동/달리기
	// 입력 잔상을 지워 계속 달리는 것을 막는다. 방향은 유지해 마찰 감속으로 자연 정지.
	void ClearMovementInput() { m_bMoved = false; m_bRunning = false; }

protected:
	// Player action
	void EnterAim();
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
	float m_fAimPitch = 0.f;

	WEAPON_TYPE m_eWeaponTypeBeforeMelee = WEAPON_TYPE::UNDEFINED;

	// 무기 슬롯 (2번 인덱스는 항상 PISTOL). m_pSlotWeapons는 로컬 플레이어만 채움 —
	// 리모트는 GiveWeapon(타입) 경로로 매번 복사본을 받는다.
	std::array<WEAPON_TYPE, 3> m_eWeaponSlots{ WEAPON_TYPE::M4, WEAPON_TYPE::AK, WEAPON_TYPE::PISTOL };
	std::array<std::shared_ptr<WeaponObject>, 3> m_pSlotWeapons{};
	int m_nCurrentWeaponSlot = 0;
	bool m_bInWeaponSwap = false;	// "Weapon Draw" 몽타주 재생 중 — 발사/근접/재장전/재교체 잠금
	bool m_bInMeleeAttack = false;
	bool m_bWasAimBeforeMelee = false;
	bool m_bMeleeStartedThisFrame = false;	// 로컬 입력으로 근접공격 시작 → 씬이 소비

	// Collision
	std::vector<BoundingOrientedBox> m_xmOBBCollided;

	// Player Components
	//std::shared_ptr<class Crosshair> m_pCrosshair = nullptr;
	PlayerHUD m_PlayerHUD{};
	std::shared_ptr<WeaponSocket> m_pWeaponSocket = nullptr;

protected:
	// Movement constants
	const float	m_fMaxMoveSpeed = 1.4_m;
	const float	m_fAcceleration = 10.0_cm;
	const float	m_fFriction = 10.f;
	const float	m_fGravity = -9.8_cm * 100;

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
	bool UsesHUD() const override { return true; }
	bool UsesInputMovement() const override { return true; }
	bool UsesServerStateMovement() const override { return false; }
	bool NeedsSendMovementState() const override { return false; }
};

class NetworkOwnerThirdPersonPlayer final : public IThirdPersonPlayer {
public:
	void ProcessInput() override;

protected:
	bool UsesLocalCamera() const override { return true; }
	bool UsesHUD() const override { return true; }
	bool UsesInputMovement() const override { return true; }
	bool UsesServerStateMovement() const override { return true; }
	bool NeedsSendMovementState() const override { return true; }

protected:
	virtual void SendLocalCommandToServer() override;
};

class NetworkRemoteThirdPersonPlayer final : public IThirdPersonPlayer {
public:
	void ProcessInput() override {}
	void Update() override;
	// receivedTime: 네트워크 스레드 패킷 도착 시각 (GetNetTimeSec 기준).
	// 소비 프레임 시각으로 스탬프하면 프레임 타이밍 노이즈가 간격 측정에 유입된다.
	void UpdateNetworkTransform(const Matrix& mtxWorld, bool bRunning, bool bAiming, float fAimPitch, float receivedTime);

	void ResetMovementState() {
		m_bMoved = false;
		m_fMoveSpeed = 0.f;
		m_fSmoothedSpeed = 0.f;
		m_bRunning = false;
	}

	float GetLastPacketTime() const { return m_fLastPacketTime; }

protected:
	bool UsesLocalCamera() const override { return false; }
	bool UsesHUD() const override { return false; }
	bool UsesInputMovement() const override { return false; }
	bool UsesServerStateMovement() const override { return false; }
	bool NeedsSendMovementState() const override { return false; }

private:
	// ── 스냅샷 보간 (좀비 ApplyServerState와 동일 방식) ─────────────────────
	// 20Hz 패킷을 그대로 SetWorldMatrix하면 패킷 사이 프레임마다 멈췄다 점프하는
	// 계단식 움직임이 된다 (관전 카메라가 붙으면 화면 전체가 떨림).
	// 도착 시각 기준으로 스냅샷을 쌓고, 매 프레임 m_fInterpDelay만큼
	// 과거 시점을 보간해 적용한다.
	struct TransformSnapshot {
		float      fTime;    // 도착 시각 (NetworkManager::GetNetTimeSec 기준)
		Vector3    v3Scale;
		Quaternion qRot;
		Vector3    v3Pos;
	};
	static constexpr float  SNAPSHOT_WINDOW   = 0.4f;  // 보관 창 (초) — delay 상한(0.25s)보다 넉넉해야 렌더 시점이 창 밖으로 밀리지 않음. 시간 기준 정리라 전송 주기와 무관
	static constexpr size_t MAX_SNAPSHOTS     = 32;    // 하드캡 (30Hz 전송 × 0.4s 창 = 12개 + 여유)
	static constexpr float  TELEPORT_DIST     = 500.f; // 패킷 1개에 이보다 크게 점프 = 순간이동으로 간주, 보간 없이 스냅 (cm)
	static constexpr float  MAX_EXTRAPOLATION = 0.1f;  // 최신 스냅샷 이후 위치 외삽 한도 (초) — 킵얼라이브(100ms) 한 번 놓친 만큼만
	static constexpr float  MOVE_START_SEGMENT = 0.05f; // 정지→이동 전환 첫 구간의 최대 길이 (초, 30Hz 전송 간격 수준) — 킵얼라이브 갭에 한 프레임 이동이 퍼져 걷기 애니가 길어지는 것 방지

	// 매 프레임 렌더 시점(현재 - m_fInterpDelay)의 보간 트랜스폼을 적용
	void ApplyInterpolatedTransform();

	std::deque<TransformSnapshot> m_Snapshots;
	float m_fInterpDelay      = 0.15f; // 패킷 간격 EMA의 1.5배로 자동 조절 (클램프 0.12~0.25s) — 하한은 킵얼라이브 간격(100ms)보다 커야 정지 직후 외삽이 안 생김
	float m_fAvgPacketInterval = 0.f; // 패킷 간격 EMA — 마지막 간격 하나로 delay를 정하면 널뛰어 위치 팝 발생
	float m_fSmoothedSpeed    = 0.f; // 보간 위치 프레임 델타 속도의 EMA — 도착 버스트로 인한 걷기 상태 깜빡임 방지
	float m_fLastPacketTime = 0.f;
};
