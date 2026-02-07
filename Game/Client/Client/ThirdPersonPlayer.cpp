#include "pch.h"
#include "ThirdPersonPlayer.h"
#include "ThirdPersonCamera.h"
#include "NodeObject.h"

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
		m_pCamera->GenerateProjectionMatrix(1.01f, 500_m, ((float)WinCore::g_dwClientWidth / (float)WinCore::g_dwClientHeight), 60.0f);
		m_pCamera->SetOwner(shared_from_this());

		// Model
		auto pModel = MODEL->Get("Ch33_nonPBR")->CopyObject<NodeObject>();
		pModel->GetTransform()->Rotate(Vector3::Up, -90.f);
		SetChild(pModel);
		//GetTransform()->Rotate(Vector3::Up, -90.f);

		// AnimationController
		AddComponent<PlayerAnimationController>();
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
}

void ThirdPersonPlayer::ProcessInput()
{
	auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto pTransform = GetTransform();
	auto pAnimationCtrl = static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		m_bMouseInUse = !m_bMouseInUse;
	}

	// Camera Rotate
	if (m_bMouseInUse) {
		HWND hWnd = ::GetActiveWindow();

		::SetCursor(NULL);

		RECT rtClientRect;
		::GetClientRect(hWnd, &rtClientRect);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.left);
		::ClientToScreen(hWnd, (LPPOINT)&rtClientRect.right);

		int nScreenCenterX = 0, nScreenCenterY = 0;
		nScreenCenterX = rtClientRect.left + WinCore::g_dwClientWidth / 2;
		nScreenCenterY = rtClientRect.top + WinCore::g_dwClientHeight / 2;

		POINT ptCursorPos;
		::GetCursorPos(&ptCursorPos);

		POINT ptDelta{ (ptCursorPos.x - nScreenCenterX), (ptCursorPos.y - nScreenCenterY) };

		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		pThirdPersonCamera->AddYaw(ptDelta.x * m_fMouseSensitivity);
		pThirdPersonCamera->AddPitch(ptDelta.y * m_fMouseSensitivity);


		::SetCursorPos(nScreenCenterX, nScreenCenterY);

		// Aim
		if (INPUT->GetButtonDown(VK_RBUTTON)) {
			m_bAiming = true;
			pThirdPersonCamera->EnterAimMode();
			pAnimationCtrl->GetMontage()->PlayMontage("Rifle Aiming Idle");
		}
		if (INPUT->GetButtonUp(VK_RBUTTON)) {
			m_bAiming = false;
			pThirdPersonCamera->LeaveAimMode();
			pAnimationCtrl->GetMontage()->StopMontage();
		}

		// Fire
		if (INPUT->GetButtonPressed(VK_LBUTTON) && m_bAiming) {
			pAnimationCtrl->GetMontage()->JumpToSection("Rifle Fire");
		}
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

void ThirdPersonPlayer::PostUpdate()
{
	auto pTransform = GetTransform();

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

	// Ground Check (not terrain)
	//Vector3 v3GroundNormal;
	//if (!m_bGrounded) {
	//	if (CheckGround(0.3f, v3GroundNormal)) {
	//		m_bGrounded = true;
	//		m_fVerticalVelocity = 0.f;
	//	}
	//}

	ImGui::Text("======= Ground Hit Result =======");
	ImGui::Text("m_bGrounded : %s", m_bGrounded ? "TRUE" : "FALSE");
	ImGui::Text("======= Terrain Hit Result =======");
	ImGui::Text("hit.bGrounded : %s", hit.bGrounded ? "TRUE" : "FALSE");
	ImGui::Text("hit.fHeight : %f", hit.fHeight);
	ImGui::Text("hit.fPenetratioon : %f", hit.fPenetrationDepth);
	ImGui::Text("hit.v3Normal : (%f, %f, %f)", hit.v3Normal.x, hit.v3Normal.y, hit.v3Normal.z);

	ApplyGravity();

	if (m_bMoved) {
		// 플레이어가 이동 방향을 바라보도록 돌린다
		float fYaw = std::atan2f(m_v3MoveDirection.x, m_v3MoveDirection.z);
		pTransform->SetRotation(0.f, fYaw, 0.f);
	}

	pTransform->Move(v3Delta, 1.f);

	if (m_bAiming) {
		auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
		Vector3 v3LookDirection = pThirdPersonCamera->GetForwardXZ();
		float fYaw = std::atan2f(v3LookDirection.x, v3LookDirection.z);
		GetTransform()->SetRotation(0.f, fYaw, 0.f);
	}

	IPlayer::PostUpdate();

	m_xmOBBCollided.clear();
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
