#include "pch.h"
#include "ThirdPersonPlayer.h"
#include "ThirdPersonCamera.h"
#include "NodeObject.h"
#include "WeaponObject.h"
#include "Sprite.h"
#include "NetworkManager.h"
#include "TextBox.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// PlayerHUD

void IThirdPersonPlayer::PlayerHUD::Initialize(const IThirdPersonPlayer & player)
{
	if (auto& pUIBoard = CUR_SCENE->GetUIBoard(); pUIBoard) {
		pCrosshair = std::make_shared<Crosshair>();
		pUIBoard->InsertUI(pCrosshair);

		pHitMarker = std::make_shared<HitMarker>();
		pUIBoard->InsertUI(pHitMarker);

		pHealthText = std::make_shared<TextBox>(L"Malgun Gothic");
		pHealthText->SetText(L"0");
		pHealthText->SetLayer(0);
		pHealthText->SetAnchor(Vector2{ 1, 1 });
		pHealthText->SetPivot(Vector2{ 1,1 });
		pHealthText->SetPosition(Vector2{ -50, -100 });
		pHealthText->SetTextHeight(50);
		pUIBoard->InsertUI(pHealthText);

		//pHealthImage

		pAmmo = std::make_shared<TextBox>(L"Malgun Gothic");
		pAmmo->SetText(L"1");
		pAmmo->SetLayer(0);
		pAmmo->SetAnchor(Vector2{ 1, 1 });
		pAmmo->SetPivot(Vector2{ 1,1 });
		pAmmo->SetPosition(Vector2{ -50, -40 });
		pAmmo->SetTextHeight(50);
		pUIBoard->InsertUI(pAmmo);

		pWeaponName = std::make_shared<TextBox>(L"Malgun Gothic");
		pWeaponName->SetText(L"2");
		pWeaponName->SetLayer(0);
		pWeaponName->SetAnchor(Vector2{ 1, 1 });
		pWeaponName->SetPivot(Vector2{ 1,1 });
		pWeaponName->SetPosition(Vector2{ -200, -40 });
		pWeaponName->SetTextHeight(50);
		pUIBoard->InsertUI(pWeaponName);

		pReloadAlert = std::make_shared<TextBox>(L"Malgun Gothic");
		pReloadAlert->SetText(L"2");
		pReloadAlert->SetLayer(0);
		pReloadAlert->SetAnchor(Vector2{ 0.5, 0.5 });
		pReloadAlert->SetPivot(Vector2{ 0,0.5 });
		pReloadAlert->SetPosition(Vector2{ 100, 0 });
		pReloadAlert->SetTextHeight(50);
		pReloadAlert->SetVisible(false);
		pUIBoard->InsertUI(pReloadAlert);
	}
}
void IThirdPersonPlayer::PlayerHUD::Update(const IThirdPersonPlayer& player)
{
	auto pWeapon = player.GetCurrentWeaponObject();

	if (pCrosshair) {
		pCrosshair->Update();
	}

	if (pHitMarker) {
		pHitMarker->Update();
	}

	if (pHealthText) {
		auto wstrHP = std::to_wstring(static_cast<int32>(player.GetHP()));
		//pHealthText->SetSize(Vector2(wstrHP.length() * 30, 50));
		pHealthText->SetText(wstrHP);
	}

	if (pHealthImage) {

	}

	if (pAmmo) {
		std::wstring wstrAmmo;
		if (pWeapon) {
			wstrAmmo = std::format(L"{} / {}", pWeapon->GetAmmoInClip(), pWeapon->GetTotalAmmo());
		}
		else {
			wstrAmmo = std::format(L"{} / {}", 0, 0);
		}

		pAmmo->SetText(wstrAmmo);
	}

	if (pReloadAlert) {
		if (pWeapon) {
			if (pWeapon->IsInReloading()) {
				pReloadAlert->SetVisible(true);
				//pReloadAlert->SetSize(Vector2{ 100,40 });
				pReloadAlert->SetText(L"Reloading");
			}
			else {
				const int32 nMaxAmmo = pWeapon->GetAmmoPerClip();
				const int32 nCurrentAmmo = pWeapon->GetAmmoInClip();
				if (nCurrentAmmo <= nMaxAmmo * 0.2) {
					pReloadAlert->SetVisible(true);
					//pReloadAlert->SetSize(Vector2{ 70,40 });
					pReloadAlert->SetText(L"Reload");
				}
				else {
					pReloadAlert->SetVisible(false);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Base ThirdPersonPlayer 

IThirdPersonPlayer::IThirdPersonPlayer()
{
}

IThirdPersonPlayer::~IThirdPersonPlayer()
{
}

void IThirdPersonPlayer::Initialize()
{
	if (!GetComponent<Transform>()) {
		AddComponent<Transform>();
	}

	if (!m_bInitialized) {
		InitializeCommonPlayer();

		if (UsesLocalCamera()) {
			InitializeLocalCamera();
		}

		if (UsesHUD()) {
			m_PlayerHUD.Initialize(*this);
		}
	}

	for (auto& component : m_pComponents) {
		if (component) {
			component->Initialize();
		}
	}

	GetTransform()->Update();

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	AddComponent<PlayerCollider>();
	GetComponent<PlayerCollider>()->Initialize();

	m_bInitialized = true;
}

void IThirdPersonPlayer::ProcessInput()
{
}

void IThirdPersonPlayer::OnMeleeEnd()
{
	PlayMeleeEndAction();
}

void IThirdPersonPlayer::OnReloadEnd()
{
	PlayReloadEndAction();
}

void IThirdPersonPlayer::ShowHitMarker()
{
	if (m_PlayerHUD.pHitMarker) {
		m_PlayerHUD.pHitMarker->Show();
	}
}

void IThirdPersonPlayer::Update()
{
	//if (m_pCrosshair) {
	//	m_pCrosshair->Update();
	//}

	m_PlayerHUD.Update(*this);

	for (const auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void IThirdPersonPlayer::OnBeginCollision(const CollisionResult& collisionResult)
{
	// 여기서는 충돌이 일어난 객체들을 모아놓고 나중에 PostUpdate에서 한번에 이동 블락 처리를 한다
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void IThirdPersonPlayer::OnWhileCollision(const CollisionResult& collisionResult)
{
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void IThirdPersonPlayer::OnEndCollision(const CollisionResult& collisionResult)
{
}

void IThirdPersonPlayer::GiveWeapon(WEAPON_TYPE eWeaponType)
{
	ApplyWeaponChanged(eWeaponType);

	// 로컬 제어 플레이어(카메라 보유)만 서버에 무기 교체 알림.
	// 리모트 플레이어는 카메라가 없으므로 재전송 루프가 생기지 않음.
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerWeapon(std::to_underlying(eWeaponType));
	}
}

void IThirdPersonPlayer::PostUpdate()
{
	if (UsesInputMovement()) {
		ApplyInputMovement();
	}
	else if (UsesServerStateMovement()) {
		ApplyServerMovementXZ();
	}

	if (m_bAiming && m_pCamera) {
		auto pThirdPersonCamera =
			std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);

		Vector3 v3LookDirection = pThirdPersonCamera->GetForwardXZ();
		float fYaw = std::atan2f(v3LookDirection.x, v3LookDirection.z);
		GetTransform()->SetRotation(0.f, fYaw, 0.f);
	}

	// 에임 회전 적용 후 전송 (회전값이 패킷에 반영되도록)
	if (NeedsSendMovementState()) {
		SendLocalCommandToServer();
	}

	IPlayer::PostUpdate();

	m_xmOBBCollided.clear();
}

void IThirdPersonPlayer::AddToQueue(OUT std::vector<IGameObject*>& pRenderQueue)
{
	if (auto p = GetCurrentWeaponObject()) {
		p->AddToQueue(pRenderQueue);
	}

	if (auto p = GetComponent<MeshRenderer>()) {
		pRenderQueue.push_back(this);
	}

	for (auto& pChild : m_pChildren) {
		pChild->AddToQueue(pRenderQueue);
	}
}

void IThirdPersonPlayer::ApplyReplicatedState(/* const ServerSidePlayerState& state */)
{
	/*
		TODO :
		1.
		서버에서 받은 패킷에서 XZ 좌표를 이용해 플레이어 위치 반영
		좌표 보간이 필요하다면 보간 이후 위치 반영을 해야함
		보간함수는 자유롭게 구현

		Y 는 클라이언트가 Terrain/Collision 여부를 보고 결정

		2.
		패킷에 yaw 정보가 있다면 캐릭터 회전에 반영
		이동중 방향 회전과 충돌하지 않아야 함

		3.
		이동/달리기/조준/사망/hp/weaponType 등을 이용하여

		// State flags
		bool m_bMoved = false;
		bool m_bAiming = false;
		bool m_bRunning = false;

		위 변수들에 적용 + 무기는 m_pWeaponSocket 에 정보 반영해야함

		예:
		if (state.aiming) EnterAim();
		else LeaveAim();

	*/
}

void IThirdPersonPlayer::ApplyReplicatedEvent(/* const ServerSidePlayerEvent& event */)
{
	/*
		TODO :
		서버 이벤트 타입에 따라 액션 함수를 호출

		발사				-> PlayFireAction()
		무기 변경			-> ApplyWeaponChanged()
		피격				-> ApplyHitReact()
		사망				-> ApplyDead()
		근접공격 시작/끝	-> PlayMeleeStartAction() / PlayMeleeEndAction()

		- 참고사항
			- 아직 피격/사망시 별도 애니메이션이나 처리가 없음
	*/
}

void IThirdPersonPlayer::ToggleMouseLook()
{
	m_bMouseInUse = !m_bMouseInUse;

	if (m_bMouseInUse) {
		OnBeginMouseLook();
	}
	else {
		OnEndMouseLook();
	}
}

void IThirdPersonPlayer::OnBeginMouseLook()
{
	UpdateMouseLookData();
	INPUT->HideCursor();
	::ClipCursor(&m_MouseClipScreenRect);
	::SetCursorPos(m_ptMouseCenterScreenPos.x, m_ptMouseCenterScreenPos.y);

	m_bSkipMouseDeltaThisFrame = true;
}

void IThirdPersonPlayer::OnEndMouseLook()
{
	::ClipCursor(nullptr);
	INPUT->ShowCursor();
}

void IThirdPersonPlayer::UpdateMouseLookData()
{
	RECT rtClientRect{};
	::GetClientRect(WinCore::g_hWnd, &rtClientRect);

	// Client center
	m_ptMouseCenterClientPos.x = (rtClientRect.right - rtClientRect.left) / 2;
	m_ptMouseCenterClientPos.y = (rtClientRect.bottom - rtClientRect.top) / 2;

	// Screen center
	m_ptMouseCenterScreenPos = m_ptMouseCenterClientPos;
	::ClientToScreen(WinCore::g_hWnd, &m_ptMouseCenterScreenPos);

	// Clip rect in screen space
	POINT ptLeftTop{};
	ptLeftTop.x = rtClientRect.left;
	ptLeftTop.y = rtClientRect.top;
	::ClientToScreen(WinCore::g_hWnd, &ptLeftTop);

	POINT ptRightBottom{};
	ptRightBottom.x = rtClientRect.right;
	ptRightBottom.y = rtClientRect.bottom;
	::ClientToScreen(WinCore::g_hWnd, &ptRightBottom);

	m_MouseClipScreenRect.left = ptLeftTop.x;
	m_MouseClipScreenRect.top = ptLeftTop.y;
	m_MouseClipScreenRect.right = ptRightBottom.x;
	m_MouseClipScreenRect.bottom = ptRightBottom.y;
}

void IThirdPersonPlayer::HandleCollision()
{
	// 문제점
	// 1. TryUp 이 ResolveCollision 에서만 호출되면서, Terrain -> Box 위로 올라설 수 없음
	// 2. Terrain 이 Box 와 Ground 를 무시하고 일정 거리 안이면 그냥 땅에 붙어버림

	auto& pTransform = GetTransform();

	const bool bWasGrounded = m_bGrounded;
	m_bGrounded = false;

	Vector3 v3Delta;
	if (m_bMoved) {
		m_fMoveSpeed += +0.5 * m_fAcceleration * m_fFriction;
		float fMaxSpeed = m_bRunning ? m_fMaxMoveSpeed * 2.f : m_fMaxMoveSpeed;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, fMaxSpeed);
	}
	else {
		m_fMoveSpeed -= 0.5 * m_fAcceleration * m_fFriction;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, m_fMaxMoveSpeed);
		if (m_fMoveSpeed <= 0.f) {
			m_v3MoveDirection = Vector3(0, 0, 0);
		}
	}
	v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y += m_fVerticalVelocity * DT;

	ResolveCollision(v3Delta);

	TerrainHit hit{};
	ResolveTerrain(v3Delta, hit, bWasGrounded);
	if (hit.bGrounded) {
		m_bGrounded = true;
		if (m_fVerticalVelocity < 0.f) {
			m_fVerticalVelocity = 0.f;
		}
	}

	pTransform->Move(v3Delta, 1.f);
}

float IThirdPersonPlayer::GetMoveSpeedXZ() const
{
	Vector3 v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y = 0.f;
	return v3Delta.Length();
}

float IThirdPersonPlayer::GetMoveSpeedSqXZ() const
{
	Vector3 v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y = 0.f;
	return v3Delta.LengthSquared();
}

void IThirdPersonPlayer::SetPlayerModel(const std::string& strModelKey)
{
	auto pModel = MODEL->LoadOrGet(strModelKey)->CopyObject<NodeObject>();
	pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
	SwapChild(0, pModel);

	if (!GetComponent<AnimationController>()) {
		AddComponent<PlayerAnimationController>();
	}

	if (!m_pWeaponSocket) {
		m_pWeaponSocket = GetComponent<Skeleton>()->CreateAttachSocket<WeaponSocket>("RightHand"s);
	}
}

void IThirdPersonPlayer::InitializeCommonPlayer()
{
	const auto& data = GCTX->GetGameData();
	auto pModel = MODEL->LoadOrGet(GameContext::g_strCharacterNames[data.m_nCurModelIndex])->CopyObject<NodeObject>();
	pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
	SetChild(pModel);

	AddComponent<PlayerAnimationController>();

	m_pWeaponSocket = GetComponent<Skeleton>()->CreateAttachSocket<WeaponSocket>("RightHand"s);
}

void IThirdPersonPlayer::InitializeLocalCamera()
{
	m_pCamera = std::make_shared<ThirdPersonCamera>();
	m_pCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
	m_pCamera->GenerateViewMatrix(
		XMFLOAT3(0.f, 0.f, -15.f),
		XMFLOAT3(0.f, 0.f, 1.f),
		XMFLOAT3(0.f, 1.f, 0.f));
	m_pCamera->GenerateProjectionMatrix(
		10.f,
		300_m,
		static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight),
		60.0f);
	m_pCamera->SetOwner(shared_from_this());
}

void IThirdPersonPlayer::ProcessLocalCameraInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);

	// Camera Rotate
	if (m_bMouseInUse) {
		if (m_bSkipMouseDeltaThisFrame) {
			m_bSkipMouseDeltaThisFrame = false;
			goto lb_breakMouseInput;
		}
		const POINT& ptCursorPos = INPUT->GetCurrentCursorPos();

		POINT ptDelta{};
		ptDelta.x = ptCursorPos.x - m_ptMouseCenterClientPos.x;
		ptDelta.y = ptCursorPos.y - m_ptMouseCenterClientPos.y;

		if (ptDelta.x != 0 || ptDelta.y != 0) {
			pThirdPersonCamera->AddYaw(static_cast<float>(ptDelta.x) * m_fMouseSensitivity);
			pThirdPersonCamera->AddPitch(static_cast<float>(ptDelta.y) * m_fMouseSensitivity);

			::SetCursorPos(m_ptMouseCenterScreenPos.x, m_ptMouseCenterScreenPos.y);
		}

	}

lb_breakMouseInput:

	// Cam rotation debug
	if (INPUT->GetButtonPressed(VK_UP)) {
		pThirdPersonCamera->AddPitch(-500.f * m_fMouseSensitivity * DT);
	}
	if (INPUT->GetButtonPressed(VK_DOWN)) {
		pThirdPersonCamera->AddPitch(500.f * m_fMouseSensitivity * DT);
	}
	if (INPUT->GetButtonPressed(VK_LEFT)) {
		pThirdPersonCamera->AddYaw(-500.f * m_fMouseSensitivity * DT);
	}
	if (INPUT->GetButtonPressed(VK_RIGHT)) {
		pThirdPersonCamera->AddYaw(500.f * m_fMouseSensitivity * DT);
	}
}

void IThirdPersonPlayer::ProcessLocalMovementInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto pAnimationCtrl = static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());
	auto pTransform = GetTransform();

	// Move
	bool bMoved = false;
	m_v3MoveDirection.x = 0.f;
	m_v3MoveDirection.z = 0.f;
	if (INPUT->GetButtonPressed('W')) {
		m_v3MoveDirection += pThirdPersonCamera->GetForwardXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('S')) {
		m_v3MoveDirection += -pThirdPersonCamera->GetForwardXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('A')) {
		m_v3MoveDirection += -pThirdPersonCamera->GetRightXZ();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('D')) {
		m_v3MoveDirection += pThirdPersonCamera->GetRightXZ();
		bMoved = true;
	}

	// 테스트용 나중에 떼버릴것
	if (INPUT->GetButtonPressed('E')) {
		m_v3MoveDirection += pTransform->GetUp();
		bMoved = true;
	}
	if (INPUT->GetButtonPressed('Q')) {
		m_v3MoveDirection += -pTransform->GetUp();
		bMoved = true;
	}

	m_bMoved = bMoved;

	// Run
	if (INPUT->GetButtonPressed(VK_LSHIFT)) {
		m_bRunning = true;
	}
	else {
		m_bRunning = false;
	}

	if (m_bMoved) {
		m_v3MoveDirection.Normalize();
	}
	else {
		m_v3MoveDirection = Vector3::Zero;
	}
}

void IThirdPersonPlayer::ProcessLocalActionInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto pAnimationCtrl = static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	// Gun Handling
	if (m_bMouseInUse) {
		// Aim
		if (INPUT->GetButtonDown(VK_RBUTTON)) {
			EnterAim();
		}

		if (INPUT->GetButtonUp(VK_RBUTTON)) {
			LeaveAim();
		}

		if ((INPUT->GetButtonDown(VK_LBUTTON) || INPUT->GetButtonPressed(VK_LBUTTON)) && m_bAiming) {
			if (!m_bInMeleeAttack) {
				m_bFiredThisFrame = m_pWeaponSocket->TryFire();
				if (m_bFiredThisFrame) {
					PlayFireAction();
				}
			}
		}

		if (m_PlayerHUD.pCrosshair && m_pWeaponSocket && m_pWeaponSocket->GetWeaponModel()) {
			if (!m_bFiredThisFrame) {
				m_PlayerHUD.pCrosshair->RemoveRecoil(
					m_pWeaponSocket->GetWeaponModel()->GetRecoilRecovery());
			}
		}

	}

	// Fire debug

	if (INPUT->GetButtonDown(VK_RSHIFT)) {
		EnterAim();
	}

	if (INPUT->GetButtonUp(VK_RSHIFT)) {
		LeaveAim();
	}

	if ((INPUT->GetButtonDown(VK_LCONTROL) || INPUT->GetButtonPressed(VK_LCONTROL)) && m_bAiming) {
		if (!m_bInMeleeAttack) {
			m_bFiredThisFrame = m_pWeaponSocket->TryFire();
			if (m_bFiredThisFrame) {
				PlayFireAction();
			}
		}
	}

	//  Melee attack
	if (INPUT->GetButtonDown('V') && !m_bInMeleeAttack) {
		PlayMeleeStartAction();
	}

	// Reloading
	if (INPUT->GetButtonDown('R')) {
		PlayReloadStartAction();
	}

}

void IThirdPersonPlayer::ApplyInputMovement()
{
	auto& pTransform = GetTransform();

	const bool bWasGrounded = m_bGrounded;
	m_bGrounded = false;

	Vector3 v3Delta;

	if (m_bMoved) {
		m_fMoveSpeed += 0.5f * m_fAcceleration * m_fFriction;
		float fMaxSpeed = m_bRunning ? m_fMaxMoveSpeed * 2.f : m_fMaxMoveSpeed;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, fMaxSpeed);
	}
	else {
		m_fMoveSpeed -= 0.5f * m_fAcceleration * m_fFriction;
		m_fMoveSpeed = std::clamp(m_fMoveSpeed, 0.f, m_fMaxMoveSpeed);

		if (m_fMoveSpeed <= 0.f) {
			m_v3MoveDirection = Vector3::Zero;
		}
	}

	v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y += m_fVerticalVelocity * DT;

	ResolveCollision(v3Delta);

	TerrainHit hit{};
	ResolveTerrain(v3Delta, hit, bWasGrounded);

	if (hit.bGrounded) {
		m_bGrounded = true;

		if (m_fVerticalVelocity < 0.f) {
			m_fVerticalVelocity = 0.f;
		}
	}

	pTransform->Move(v3Delta, 1.f);

	float fDeltaY = std::abs(v3Delta.y);
	m_bMoved |= (fDeltaY >= 0.1);

	//ImGui::Text("Moved? : %s", m_bMoved ? "T" : "F");

	ApplyGravity();

	if (m_bMoved) {
		float fYaw = std::atan2f(m_v3MoveDirection.x, m_v3MoveDirection.z);
		pTransform->SetRotation(0.f, fYaw, 0.f);
	}
}

void IThirdPersonPlayer::ApplyServerMovementXZ()
{
	if (!m_bHasServerTargetXZ) {
		m_bMoved = false;
		return;
	}

	auto pTransform = GetTransform();
	Vector3 v3Current = pTransform->GetPosition();

	Vector3 v3Delta = Vector3::Zero;
	v3Delta.x = m_v2ServerTargetXZ.x - v3Current.x;
	v3Delta.z = m_v2ServerTargetXZ.y - v3Current.z;
	v3Delta.y = 0.f;

	float fLenSq = v3Delta.x * v3Delta.x + v3Delta.z * v3Delta.z;
	m_bMoved = fLenSq > 0.0001f;

	if (m_bMoved) {
		m_v3MoveDirection = Vector3(v3Delta.x, 0.f, v3Delta.z);
		m_v3MoveDirection.Normalize();

		m_fMoveSpeed = std::sqrt(fLenSq) / std::max(DT, 0.0001f);
	}
	else {
		m_v3MoveDirection = Vector3::Zero;
		m_fMoveSpeed = 0.f;
	}

	ResolveGroundYOnly(v3Delta);

	pTransform->Move(v3Delta, 1.f);

	if (m_bMoved && !m_bAiming) {
		float fYaw = std::atan2f(m_v3MoveDirection.x, m_v3MoveDirection.z);
		pTransform->SetRotation(0.f, fYaw, 0.f);
	}
}

void IThirdPersonPlayer::SetServerTargetXZ(float x, float z)
{
	m_v2ServerTargetXZ = Vector2(x, z);
	m_bHasServerTargetXZ = true;
}

void IThirdPersonPlayer::ResolveGroundYOnly(Vector3& delta)
{
	TerrainHit hit{};

	ResolveTerrain(delta, hit, false);

	if (hit.bGrounded) {
		m_bGrounded = true;
		m_fVerticalVelocity = 0.f;
	}
	else {
		m_bGrounded = false;

		delta.y += m_fVerticalVelocity * DT;
		ApplyGravity();
	}
}

void IThirdPersonPlayer::EnterAim()
{
	// 근접공격 중에는 즉시 aim 진입을 보류한다.
	// 여기서 aim 몽타주를 재생하면 "Melee Attack" 몽타주가 끊겨 종료 콜백
	// (PlayMeleeEndAction)이 호출되지 않아 m_bInMeleeAttack 이 영구히 true 로 남는다.
	// 대신 예약만 해두고, 근접 종료 시 PlayMeleeEndAction 이 aim 으로 진입한다.
	if (m_bInMeleeAttack) {
		m_bWasAimBeforeMelee = true;
		return;
	}

	if (m_bAiming) {
		return;
	}

	m_bAiming = true;

	if (m_pCamera) {
		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		pThirdPersonCamera->EnterAimMode();
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	if (GetCurrentWeaponObject()->IsInReloading() == false) {
		if (m_pWeaponSocket->GetCurrentWeaponType() != WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->PlayMontage("Rifle Aiming Idle");
		}
		else {
			pAnimationCtrl->GetMontage()->PlayMontage("Pistol Aiming Idle");
		}
	}

	if (m_PlayerHUD.pCrosshair) {
		m_PlayerHUD.pCrosshair->SetVisible(true);
	}
}

void IThirdPersonPlayer::LeaveAim()
{
	// 근접공격 중 우클릭을 뗀 경우: aim 예약만 취소(근접 몽타주는 그대로 둔다).
	if (m_bInMeleeAttack) {
		m_bWasAimBeforeMelee = false;
		return;
	}

	if (!m_bAiming) {
		return;
	}

	m_bAiming = false;

	if (m_pCamera) {
		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		pThirdPersonCamera->LeaveAimMode();
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	// TODO:
	// 현재 StopMontage()는 조준 idle뿐 아니라 fire/melee montage도 끊을 수 있음.
	// 서버 이벤트 기반 애니메이션이 들어오면 Aim montage만 멈추는 방식으로 분리 필요.
	if (!GetCurrentWeaponObject()->IsInReloading() && !m_bInMeleeAttack) {
		pAnimationCtrl->GetMontage()->StopMontage();
	}

	if (m_PlayerHUD.pCrosshair) {
		m_PlayerHUD.pCrosshair->SetVisible(false);
	}
}

void IThirdPersonPlayer::PlayFireAction()
{
	if (!m_pWeaponSocket) {
		return;
	}

	m_bFiredThisFrame = true;

	auto pAnimationCtrl = std::static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	if (m_pWeaponSocket->GetCurrentWeaponType() != WEAPON_TYPE::PISTOL) {
		pAnimationCtrl->GetMontage()->JumpToSection("Rifle Fire");
	}
	else {
		pAnimationCtrl->GetMontage()->JumpToSection("Pistol Fire");
	}

	if (m_PlayerHUD.pCrosshair && m_pWeaponSocket->GetWeaponModel()) {
		m_PlayerHUD.pCrosshair->AddRecoil(m_pWeaponSocket->GetWeaponModel()->GetRecoil());
	}

	//if (m_PlayerHUD.pHitMarker && m_pWeaponSocket->GetWeaponModel()) {
	//	m_PlayerHUD.pHitMarker->Show();
	//}

}

void IThirdPersonPlayer::PlayMeleeStartAction()
{
	if (m_bInMeleeAttack) {
		return;
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	m_eWeaponTypeBeforeMelee = m_pWeaponSocket->GetCurrentWeaponType();
	m_bWasAimBeforeMelee = m_bAiming;
	m_bInMeleeAttack = true;
	m_bMeleeStartedThisFrame = true;	// 씬에서 소비 → 서버 전송(온라인)/로컬 판정(오프라인)

	ApplyWeaponChanged(WEAPON_TYPE::MELEE);
	pAnimationCtrl->GetMontage()->PlayMontage("Melee Attack");
}

void IThirdPersonPlayer::PlayMeleeEndAction()
{
	auto pAnimationCtrl = std::static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	ApplyWeaponChanged(m_eWeaponTypeBeforeMelee);
	pAnimationCtrl->GetMontage()->StopMontage();

	m_bInMeleeAttack = false;

	if (m_bWasAimBeforeMelee) {
		EnterAim();
	}
}

void IThirdPersonPlayer::PlayReloadStartAction()
{
	auto pWeapon = GetCurrentWeaponObject();
	if (pWeapon->IsInReloading() == true) return;

	std::cout << ">>> [Player] PlayReloadStartAction called (Local: " << (GetCamera() ? "YES" : "NO") << ") <<<\n";

	// 로컬 플레이어라면 서버에 재장전 알림
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerReload();
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	auto pMontage = pAnimationCtrl->GetMontage().get();
	if (m_bAiming && pMontage->IsPlaying()) {
		if (m_pWeaponSocket->GetCurrentWeaponType() == WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->JumpToSection("Pistol Reloading");
		}
		else {
			pAnimationCtrl->GetMontage()->JumpToSection("Rifle Reloading");
		}

		//pMontage->JumpToSection("Rifle Reloading");
	}
	else {
		if (m_pWeaponSocket->GetCurrentWeaponType() == WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->PlayMontage("Pistol Reloading");
		}
		else {
			pAnimationCtrl->GetMontage()->PlayMontage("Rifle Reloading");
		}

		//pMontage->PlayMontage("Rifle Reloading");
	}
	pWeapon->BeginReload();
}

void IThirdPersonPlayer::PlayReloadEndAction()
{
	GetCurrentWeaponObject()->EndReload();

	auto pAnimationCtrl = std::static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	if (m_bAiming) {
		if (m_pWeaponSocket->GetCurrentWeaponType() != WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->JumpToSection("Rifle Aiming Idle");
		}
		else {
			pAnimationCtrl->GetMontage()->JumpToSection("Pistol Aiming Idle");
		}
	}
	else {
		pAnimationCtrl->GetMontage()->StopMontage();
	}
}

void IThirdPersonPlayer::ApplyWeaponChanged(WEAPON_TYPE eWeaponType)
{
	auto eBefore = m_pWeaponSocket->GetCurrentWeaponType();
	// montage 점프는 로컬 플레이어(카메라 보유)의 조준 보정 전용.
	// 리모트는 조준/근접/발사 montage가 동기화 이벤트로 구동되므로 여기서 건드리면
	// 진행 중인 montage의 종료 notify가 누락되어 조준/근접 상태가 안 풀린다.
	if (eBefore != eWeaponType && m_bAiming && m_pCamera) {
		auto pAnimationCtrl = static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());
		if (eWeaponType == WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->JumpToSection("Pistol Aiming Idle");
		}
		else if (eBefore == WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->JumpToSection("Rifle Aiming Idle");
		}
	}

	m_pWeaponSocket->SetWeapon(eWeaponType);
	if (m_PlayerHUD.pWeaponName) {
		const std::string& strWeaponName = GameContext::g_strWeaponNames[std::to_underlying(eWeaponType)];
		m_PlayerHUD.pWeaponName->SetText(::StringToWString(strWeaponName));
	}
}

void IThirdPersonPlayer::ApplyHitReact(float damage)
{

}

void IThirdPersonPlayer::ApplyDead()
{

}

void IThirdPersonPlayer::ApplyGravity()
{
	if (CUR_SCENE->IsGravityOn()) {
		if (!m_bGrounded) {
			m_fVerticalVelocity += m_fGravity * DT;
		}
		else {
			m_fVerticalVelocity = 0.f;
		}
	}
}

void IThirdPersonPlayer::ResolveCollision(OUT Vector3& outv3Delta)
{
	const BoundingCapsule& capsuleWorld = GetComponent<PlayerCollider>()->GetCapsuleWorld();

	const uint32 unPassCount = 2;
	const float fSkin = 0.5f;
	const float fGround = 0.7f;
	const float fSnapDistance = 1.0f;

	for (uint32 pass = 0; pass < unPassCount; ++pass) {
		bool bAnyHit = false;

		for (auto& xmOBB : m_xmOBBCollided) {
			Vector3 v3Normal;
			float fDepth;
			if (!capsuleWorld.Intersects(xmOBB, v3Normal, fDepth)) {
				continue;
			}

			if (fDepth < fSkin) {
				continue;
			}

			if (v3Normal.y > fGround && outv3Delta.y <= 0.f) {
				// 바닥 접촉 확정
				m_bGrounded = true;
				m_unGroundGraceFrames = m_unMaxGroundGraceFrames;

				if (m_fVerticalVelocity < 0.f) {
					m_fVerticalVelocity = 0.f;
				}

				// Penetration Correction
				if (fDepth > fSnapDistance) {
					float fPush = std::min(fDepth + fSkin, 5.f);
					outv3Delta += v3Normal * fPush;
				}

				// Slope Projection
				float fProjected = outv3Delta.Dot(v3Normal);
				if (fProjected < 0.f) {
					outv3Delta -= v3Normal * fProjected;
				}

				continue;
			}

			float fProjectedAmount = outv3Delta.Dot(v3Normal);
			if (fProjectedAmount < 0.f) {
				// Step
				if (TryStepUp(capsuleWorld, xmOBB, outv3Delta)) {
					bAnyHit = true;
					continue;
				}

				// Wall
				outv3Delta -= v3Normal * fProjectedAmount;
				bAnyHit = true;
			}
		}

		if (!bAnyHit) {
			break;
		}
	}
}

bool IThirdPersonPlayer::CheckGround(float fMaxDistance, OUT Vector3& outv3Normal)
{
	const BoundingCapsule& capsuleWorld = GetComponent<PlayerCollider>()->GetCapsuleWorld();
	const float fProbe = fMaxDistance;

	for (auto& xmOBB : m_xmOBBCollided) {
		Vector3 v3Normal;
		float fDepth;

		BoundingCapsule test = capsuleWorld;
		test.v3Center += Vector3::Down * fProbe;
		if (test.Intersects(xmOBB, v3Normal, fDepth)) {
			if (v3Normal.y > 0.6f) {
				outv3Normal = v3Normal;
				return true;
			}
		}
	}
	return false;
}

bool IThirdPersonPlayer::TryStepUp(const BoundingCapsule& capsule, const BoundingOrientedBox& box, OUT Vector3& outv3Delta)
{
	// Try move up
	Vector3 v3Up = Vector3(0.f, m_fStepHeight, 0.f);
	Vector3 v3StepDelta = outv3Delta + v3Up;

	BoundingCapsule testCapsule = capsule;
	testCapsule.v3Center += v3StepDelta;

	// Collision Check
	Vector3 v3Normal;
	float fDepth;
	if (testCapsule.Intersects(box, v3Normal, fDepth)) {
		return false;	// Cannot Step up
	}

	v3StepDelta.y -= m_fStepHeight;
	outv3Delta = v3StepDelta;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// LocalThirdPersonPlayer

void LocalThirdPersonPlayer::ProcessInput()
{
	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		ToggleMouseLook();
	}

	ProcessLocalCameraInput();
	ProcessLocalMovementInput();
	ProcessLocalActionInput();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// NetworkOwnerThirdPersonPlayer

void NetworkOwnerThirdPersonPlayer::ProcessInput()
{
	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		ToggleMouseLook();
	}

	// 카메라는 클라가 돌림
	ProcessLocalCameraInput();
	ProcessLocalMovementInput();
	ProcessLocalActionInput();
}

void NetworkOwnerThirdPersonPlayer::SendLocalCommandToServer()
{
	// 이동, 조준, 달리기 상태가 변경되었을 때 패킷 전송
	// 매 프레임 호출되므로 상태 변화가 있을 때만 보내는 것이 효율적이지만, 
	// 여기서는 단순함을 위해 m_bMoved 조건에 m_bAiming 등을 추가하거나 항상 보내도록 수정
	
	static bool bLastAiming = false;
	static bool bLastRunning = false;
	
	// 상태 변화 체크 (또는 움직임 체크)
	bool bStateChanged = (bLastAiming != m_bAiming) || (bLastRunning != m_bRunning);
	
	if (m_bMoved || bStateChanged || m_bAiming) {
		C2S_Transform packet;
		packet.size = sizeof(C2S_Transform);
		packet.type = C2S_TRANSFORM;
		
		memcpy(&packet.transform.m, &GetTransform()->GetWorldMatrix(), sizeof(float) * 16);
		packet.bRunning = m_bRunning;
		packet.bAiming = m_bAiming;
		
		auto pCamera = std::static_pointer_cast<ThirdPersonCamera>(GetCamera());
		if (pCamera) packet.fAimPitch = pCamera->GetPitch();
		else packet.fAimPitch = 0.f;

		NetworkManager::GetInstance()->SendPacket(&packet, packet.size);
		
		bLastAiming = m_bAiming;
		bLastRunning = m_bRunning;
	}
}

void NetworkRemoteThirdPersonPlayer::UpdateNetworkTransform(const Matrix& mtxWorld, bool bRunning, bool bAiming, float fAimPitch)
{
	auto pTransform = GetTransform();

	// 상태 플래그는 지터 필터와 무관하게 항상 최신값 적용
	m_bRunning = bRunning;
	m_fAimPitch = fAimPitch;

	if (bAiming) {
		EnterAim();
	}
	else {
		LeaveAim();
	}

	pTransform->SetWorldMatrix(mtxWorld);

	// 이동 속도 계산은 지터 필터 적용
	float fCurrentTime = TIME->GetTotalTime();
	float fActualDT = fCurrentTime - m_fLastPacketTime;

	if (fActualDT < 0.01f) {
		return;
	}

	m_fLastPacketTime = fCurrentTime;

	Vector3 v3PrevPos = pTransform->GetPosition();
	Vector3 v3Delta = mtxWorld.Translation() - v3PrevPos;
	v3Delta.y = 0.f;

	float fDistSq = v3Delta.LengthSquared();

	if (fDistSq > 0.000001f) {
		m_bMoved = true;
		m_v3MoveDirection = v3Delta;
		m_v3MoveDirection.Normalize();
		m_fMoveSpeed = std::sqrt(fDistSq) / fActualDT;
	}
	else {
		m_bMoved = false;
		m_fMoveSpeed = 0.f;
	}
}
