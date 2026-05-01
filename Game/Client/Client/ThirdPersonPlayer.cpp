#include "pch.h"
#include "ThirdPersonPlayer.h"
#include "ThirdPersonCamera.h"
#include "NodeObject.h"
#include "Sprite.h"

ThirdPersonPlayer::ThirdPersonPlayer()
{
}

ThirdPersonPlayer::~ThirdPersonPlayer()
{
}

void ThirdPersonPlayer::Initialize()
{
	if (!m_bInitialized) {
		// Camera
		m_pCamera = std::make_shared<ThirdPersonCamera>();
		m_pCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
		m_pCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);
		m_pCamera->GenerateViewMatrix(XMFLOAT3(0.f, 0.f, -15.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_pCamera->GenerateProjectionMatrix(10.f, 300_m, ((float)WinCore::g_dwClientWidth / (float)WinCore::g_dwClientHeight), 60.0f);
		m_pCamera->SetOwner(shared_from_this());

		// Model
		auto pModel = MODEL->LoadOrGet("Ch33_nonPBR")->CopyObject<NodeObject>();
		pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
		SetChild(pModel);
		//GetTransform()->Rotate(Vector3::Up, -90.f);

		// AnimationController
		AddComponent<PlayerAnimationController>();

		if (auto& pUIBoard = CUR_SCENE->GetUIBoard(); pUIBoard) {
			m_pCrosshair = std::make_shared<Crosshair>();
			pUIBoard->InsertUI(m_pCrosshair);
		}

		m_pWeaponSocket = GetComponent<Skeleton>()->CreateAttachSocket<WeaponSocket>("RightHand"s);
	}

	for (auto& component : m_pComponents) {
		if (component) {
			component->Initialize();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	AddComponent<PlayerCollider>();
	GetComponent<PlayerCollider>()->Initialize();

	m_bInitialized = true;
}

void ThirdPersonPlayer::ProcessInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto pTransform = GetTransform();
	auto pAnimationCtrl = static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		ToggleMouseLook();
	}

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

		// Aim
		if (INPUT->GetButtonDown(VK_RBUTTON)) {
			m_bAiming = true;
			pThirdPersonCamera->EnterAimMode();
			pAnimationCtrl->GetMontage()->PlayMontage("Rifle Aiming Idle");

			if (m_pCrosshair) {
				m_pCrosshair->SetVisible(true);
			}
		}

		if (INPUT->GetButtonUp(VK_RBUTTON)) {
			m_bAiming = false;
			pThirdPersonCamera->LeaveAimMode();
			pAnimationCtrl->GetMontage()->StopMontage();

			if (m_pCrosshair) {
				m_pCrosshair->SetVisible(false);
			}
		}

		// Fire
		if (INPUT->GetButtonPressed(VK_LBUTTON) && m_bAiming) {
			if (m_pWeaponSocket->TryFire()) {
				pAnimationCtrl->GetMontage()->JumpToSection("Rifle Fire");
				m_bFiredThisFrame = true;
			}
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

	m_v3MoveDirection.Normalize();
}

void ThirdPersonPlayer::Update()
{
	for (const auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void ThirdPersonPlayer::OnBeginCollision(const CollisionResult& collisionResult)
{
	// 여기서는 충돌이 일어난 객체들을 모아놓고 나중에 PostUpdate에서 한번에 이동 블락 처리를 한다
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void ThirdPersonPlayer::OnWhileCollision(const CollisionResult& collisionResult)
{
	m_xmOBBCollided.push_back(collisionResult.DecomposeRef().second.GetOBBWorld());
}

void ThirdPersonPlayer::OnEndCollision(const CollisionResult& collisionResult)
{
}

void ThirdPersonPlayer::GiveWeapon(WEAPON_TYPE eWeaponType)
{
	m_pWeaponSocket->SetWeapon(eWeaponType);
}

void ThirdPersonPlayer::PostUpdate()
{
	auto& pTransform = GetTransform();

	HandleCollision();
	ApplyGravity();

	if (m_bMoved) {
		// 플레이어가 이동 방향을 바라보도록 돌린다
		float fYaw = std::atan2f(m_v3MoveDirection.x, m_v3MoveDirection.z);
		pTransform->SetRotation(0.f, fYaw, 0.f);
	}

	if (m_bAiming) {
		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		Vector3 v3LookDirection = pThirdPersonCamera->GetForwardXZ();
		float fYaw = std::atan2f(v3LookDirection.x, v3LookDirection.z);
		GetTransform()->SetRotation(0.f, fYaw, 0.f);
	}

	IPlayer::PostUpdate();

	m_xmOBBCollided.clear();
}

void ThirdPersonPlayer::ToggleMouseLook()
{
	m_bMouseInUse = !m_bMouseInUse;

	if (m_bMouseInUse) {
		OnBeginMouseLook();
	}
	else {
		OnEndMouseLook();
	}
}

void ThirdPersonPlayer::OnBeginMouseLook()
{
	UpdateMouseLookData();
	INPUT->HideCursor();
	::ClipCursor(&m_MouseClipScreenRect);
	::SetCursorPos(m_ptMouseCenterScreenPos.x, m_ptMouseCenterScreenPos.y);

	m_bSkipMouseDeltaThisFrame = true;
}

void ThirdPersonPlayer::OnEndMouseLook()
{
	::ClipCursor(nullptr);
	INPUT->ShowCursor();
}

void ThirdPersonPlayer::UpdateMouseLookData()
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

void ThirdPersonPlayer::HandleCollision()
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

float ThirdPersonPlayer::GetMoveSpeedXZ() const
{
	Vector3 v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y = 0.f;
	return v3Delta.Length();
}

float ThirdPersonPlayer::GetMoveSpeedSqXZ() const
{
	Vector3 v3Delta = m_v3MoveDirection * (m_fMoveSpeed * DT);
	v3Delta.y = 0.f;
	return v3Delta.LengthSquared();
}

void ThirdPersonPlayer::ApplyGravity()
{
	if (!m_bGrounded) {
		m_fVerticalVelocity += m_fGravity * DT; 
	}
	else {
		m_fVerticalVelocity = 0.f;
	}
}

void ThirdPersonPlayer::ResolveCollision(OUT Vector3& outv3Delta)
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

bool ThirdPersonPlayer::CheckGround(float fMaxDistance, OUT Vector3& outv3Normal)
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

bool ThirdPersonPlayer::TryStepUp(const BoundingCapsule& capsule, const BoundingOrientedBox& box, OUT Vector3& outv3Delta)
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
