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
class ImageBox;


class IThirdPersonPlayer : public IPlayer {
private:
	struct PlayerHUD {
		std::shared_ptr<Crosshair>	pCrosshair = nullptr;
		std::shared_ptr<HitMarker>	pHitMarker = nullptr;
		std::shared_ptr<TextBox>	pHealthText = nullptr;
		std::shared_ptr<ImageBox>	pHealthImage = nullptr;	// 체력 숫자 왼쪽 십자 아이콘
		std::shared_ptr<TextBox>	pAmmo = nullptr;
		std::shared_ptr<TextBox>	pWeaponName = nullptr;
		std::shared_ptr<TextBox>	pReloadAlert = nullptr;
		std::shared_ptr<ImageBox>	pBandageCastGauge = nullptr;	// 붕대 캐스트 라디얼 게이지 (화면 중앙, 남은 시간만큼 원형으로 줄어듦)
		std::shared_ptr<TextBox>	pBandageCastText = nullptr;		// 게이지 아래 퍼센트 표시
		std::shared_ptr<TextBox>	pBandageHelpText = nullptr;		// 붕대 들기 중 사용법 안내 (화면 하단 중앙)

		// 무기 슬롯 UI — 슬롯별 아이콘 + 번호, 현재 슬롯 하이라이트
		std::shared_ptr<ImageBox>	pSlotImages[3] = {};
		std::shared_ptr<TextBox>	pSlotLabels[3] = {};
		WEAPON_TYPE					eSlotIconTypes[3] = { WEAPON_TYPE::COUNT, WEAPON_TYPE::COUNT, WEAPON_TYPE::COUNT };	// 아이콘 교체 감지용

		// 소모품 슬롯 UI — 붕대 / 폭발 수류탄 / 디코이 수류탄 (아이콘 + 보유 개수)
		std::shared_ptr<ImageBox>	pItemImages[3] = {};
		std::shared_ptr<TextBox>	pItemCounts[3] = {};

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
	// 붕대 사용 요청 소비 (0=없음, 1=자기 회복, 2=타인 회복) — 씬이 대상 해석 후 StartBandageCast 호출
	int  ConsumeBandageRequest() { int n = m_nBandageRequest; m_nBandageRequest = 0; return n; }
	bool IsHoldingBandage() const { return m_bHoldingBandage; }
	bool IsBandaging() const { return m_bInBandage; }
	int  GetBandageCount() const { return m_nBandageCount; }
	float GetBandageProgress() const { return m_bInBandage ? m_fBandageTimer / BANDAGE_CAST_SECONDS : 0.f; }	// 0~1
	bool IsHoldingGrenade() const { return m_bHoldingGrenade; }
	bool IsGrenadeWindup() const { return m_bGrenadeWindup; }
	bool IsGrenadeThrowing() const { return m_bInGrenadeThrow; }
	int  GetGrenadeCount() const { return m_nGrenadeCount; }
	int  GetDecoyCount() const { return m_nDecoyCount; }
	bool IsDecoySelected() const { return m_bDecoySelected; }	// 현재 든 수류탄이 디코이인지

	// "Throw" 원본 클립 기준 비율 — 몽타주 빌드/일시정지 공용 (클립 보고 튜닝)
	static constexpr float GRENADE_HOLD_RATIO    = 0.25f;	// 뒤로 든 자세에서 일시정지하는 지점
	static constexpr float GRENADE_RELEASE_RATIO = 0.45f;	// 투사체 릴리즈 notify (홀드 지점보다 커야 함)
	// 릴리즈 notify가 세운 투척 요청 소비 — 씬이 투사체 스폰 + 전송 담당
	bool ConsumeGrenadeRelease() { bool b = m_bGrenadeReleasePending; m_bGrenadeReleasePending = false; return b; }

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
	void OnGrenadeRelease();	// "Grenade Throw" 릴리즈 notify — 투척 요청 플래그 (로컬만 씬이 소비)
	void OnGrenadeThrowEnd();	// "Grenade Throw" 종료 notify — 잠금 해제, 개수 0이면 무기 복귀
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
	void PlayBandageStartAction(bool bAllyTarget = false);	// public: 리모트 플레이어 붕대 모션 재생에도 사용 (아군 대상이면 전용 모션)
	void StopBandageAction();		// public: 리모트 붕대 모션 종료(취소/완료 패킷)에도 사용
	void PlayBandageHoldAction();	// public: 리모트 붕대 들기 (총 내림 + 꺼내는 모션) — state 3
	void PlayBandageUnholdAction();	// public: 리모트 붕대 내리기 (총 복귀 + 꺼내는 모션) — state 4
	void PlayGrenadeEquipAction();		// public: 리모트 수류탄 들기 (총 내림) — state 3
	void PlayGrenadeUnequipAction();	// public: 리모트 수류탄 내리기 (총 복귀) — state 4
	void PlayGrenadeWindupAction();		// public: 리모트 와인드업 ("Grenade Hold" 몽타주) — state 2
	void PlayGrenadeThrowAction();		// public: 리모트 던지기 ("Grenade Throw" 몽타주) — state 0
	void StartBandageCast(int targetPlayerId);	// 로컬: 대상 확정 후 4초 캐스트 시작 (targetPlayerId=-1 → 오프라인 자기)
	void CancelBandage();		// 캐스트 중단 (이동/사망/대상 이탈) — 온라인이면 취소 패킷 전송
	int  GetBandageTargetId() const { return m_nBandageTargetId; }
	void LeaveAim();				// public: 컷씬 진입 시 Scene::PushCinematic이 조준 해제에 사용 (멱등 — 비조준이면 no-op)

	// 입력 처리가 차단된 프레임(컷씬/로딩 대기)에 호출 — 직전 프레임의 이동/달리기
	// 입력 잔상을 지워 계속 달리는 것을 막는다. 방향은 유지해 마찰 감속으로 자연 정지.
	void ClearMovementInput() { m_bMoved = false; m_bRunning = false; }
	void SetMouseLookEnabled(bool bEnable);

protected:
	// Player action
	void EnterAim();
	void PlayMeleeEndAction();

	// 붕대 모드/캐스트 (4번 키 = 붕대 들기, 좌클릭=자기/우클릭=타인 회복)
	void EnterBandageMode();			// 총 내림 + 붕대 꺼내는 모션 ("Bandage Draw")
	void ExitBandageToSlot(int nSlot);	// 붕대 모드 해제 + 요청 슬롯 무기 꺼내는 모션. 비모드면 SelectWeaponSlot 위임. 캐스트 중 무시
	void FinishBandage();		// 4초 완료 — 로컬: 소모+회복(오프라인)/서버 전송(온라인), 리모트: 모션 종료만

	// 수류탄 모드 (5번 키 = 들기, 좌클릭 꾹 = 와인드업, 뗌 = 던지기)
	void EnterGrenadeMode(bool bDecoy = false);	// 총 내림 + 꺼내는 모션 ("Bandage Draw" 재사용). bDecoy=디코이 수류탄 선택
	void ExitGrenadeToSlot(int nSlot);	// 수류탄 모드 해제 + 요청 슬롯 무기 복귀. 비모드면 SelectWeaponSlot 위임. 던지는 중 무시
	void StartGrenadeWindup();			// 좌클릭 누름 — "Grenade Hold" 몽타주 (마지막 자세 유지)
	void StartGrenadeThrow();			// 좌클릭 뗌 — "Grenade Throw" 몽타주, 릴리즈 notify가 투척 요청
	void SetGrenadeHandVisible(bool bVisible);	// 오른손 수류탄 모델 표시/숨김 (첫 표시 때 소켓 생성)
	void SetBandageHandVisible(bool bVisible);	// 오른손 붕대 모델 표시/숨김 (첫 표시 때 소켓 생성)
public:
	std::shared_ptr<GrenadeHandSocket> GetGrenadeSocket() const { return m_pGrenadeSocket; }	// 오프셋 튜닝 ImGui용

	void ApplyWeaponChanged(WEAPON_TYPE eWeaponType);
	void ApplyHitReact(float damage);
	void ApplyDead();		// 사망 모션 재생 (FREEZE) + 진행 중 액션/잠금 정리 — 로컬/리모트 공용
	void ApplyRespawn();	// 부활 — 사망 몽타주 해제, 상태머신 복귀

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

	// ── 붕대 (소모품 슬롯 0번) ───────────────────────────────────────────────
	static constexpr float BANDAGE_CAST_SECONDS = 4.f;
	static constexpr float BANDAGE_HEAL_AMOUNT  = 50.f;
	static constexpr int   BANDAGE_START_COUNT  = 3;
	bool  m_bHoldingBandage  = false;	// 붕대 들기 모드 (조준/발사/근접/재장전 잠금)
	bool  m_bInBandage       = false;	// 감기 캐스트 진행 중 (로컬+리모트 공용, Update가 타이머 진행)
	float m_fBandageTimer    = 0.f;
	int   m_nBandageTargetId = -1;		// 회복 대상 playerId (-1 = 오프라인 자기)
	int   m_nBandageCount    = BANDAGE_START_COUNT;
	int   m_nBandageRequest  = 0;		// 0=없음, 1=자기, 2=타인 — 씬이 소비

	// ── 수류탄 (소모품 슬롯 1번) ─────────────────────────────────────────────
	static constexpr int GRENADE_START_COUNT = 3;
	static constexpr int DECOY_START_COUNT   = 3;
	bool m_bHoldingGrenade       = false;	// 수류탄 들기 모드 (조준/발사/근접/재장전 잠금)
	bool m_bGrenadeWindup        = false;	// 좌클릭 꾹 — 뒤로 들고 있는 중
	bool m_bInGrenadeThrow       = false;	// "Grenade Throw" 몽타주 재생 중 (모드 전환 잠금)
	bool m_bGrenadeReleasePending = false;	// 릴리즈 notify → 씬이 소비해 투사체 스폰
	int  m_nGrenadeCount         = GRENADE_START_COUNT;
	int  m_nDecoyCount           = DECOY_START_COUNT;
	bool m_bDecoySelected        = false;	// 들기 모드에서 선택된 수류탄 종류 (false=프래그, true=디코이)

	// Collision
	std::vector<BoundingOrientedBox> m_xmOBBCollided;

	// Player Components
	//std::shared_ptr<class Crosshair> m_pCrosshair = nullptr;
	PlayerHUD m_PlayerHUD{};
	std::shared_ptr<WeaponSocket> m_pWeaponSocket = nullptr;
	std::shared_ptr<GrenadeHandSocket> m_pGrenadeSocket = nullptr;	// 손 수류탄 모델 (지연 생성)
	std::shared_ptr<BandageHandSocket> m_pBandageSocket = nullptr;	// 손 붕대 모델 (지연 생성)

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
