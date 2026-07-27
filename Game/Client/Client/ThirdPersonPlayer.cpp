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

		pHealthText = std::make_shared<TextBox>(L"Noto Sans KR");
		pHealthText->SetText(L"0");
		pHealthText->SetLayer(0);
		pHealthText->SetAnchor(Vector2{ 1, 1 });
		pHealthText->SetPivot(Vector2{ 0,1 });	// 좌하단 피벗 — 아이콘 옆에 왼쪽 정렬로 붙임
		pHealthText->SetPosition(Vector2{ -170, -100 });
		pHealthText->SetTextHeight(50);
		pUIBoard->InsertUI(pHealthText);

		// HP 아이콘 — 체력 숫자 왼쪽
		pHealthImage = std::make_shared<ImageBox>("../Resources/Textures/health.png");
		pHealthImage->SetLayer(0);
		pHealthImage->SetAnchor(Vector2{ 1, 1 });
		pHealthImage->SetPivot(Vector2{ 1, 1 });
		pHealthImage->SetPosition(Vector2{ -175.f, -103.f });
		pHealthImage->SetSize(Vector2{ 44, 44 });
		pUIBoard->InsertUI(pHealthImage);

		pAmmo = std::make_shared<TextBox>(L"Noto Sans KR");
		pAmmo->SetText(L"1");
		pAmmo->SetLayer(0);
		pAmmo->SetAnchor(Vector2{ 1, 1 });
		pAmmo->SetPivot(Vector2{ 1,1 });
		pAmmo->SetPosition(Vector2{ -50, -40 });
		pAmmo->SetTextHeight(50);
		pUIBoard->InsertUI(pAmmo);

		pWeaponName = std::make_shared<TextBox>(L"Noto Sans KR");
		pWeaponName->SetText(L"2");
		pWeaponName->SetLayer(0);
		pWeaponName->SetAnchor(Vector2{ 1, 1 });
		pWeaponName->SetPivot(Vector2{ 1,1 });
		pWeaponName->SetPosition(Vector2{ -200, -40 });
		pWeaponName->SetTextHeight(50);
		pUIBoard->InsertUI(pWeaponName);

		// 무기 슬롯 UI — 슬롯별 아이콘 + 번호, 현재 슬롯 하이라이트 (권총은 작게)
		constexpr float fSlotY = -160.f;
		constexpr float fSlotGap = 20.f;
		const Vector2 v2SlotSizes[3] = { { 150.f, 75.f }, { 150.f, 75.f }, { 100.f, 50.f } };
		float fRight = -20.f;
		constexpr float fMaxSlotH = 75.f;
		for (int i = 2; i >= 0; --i) {
			const Vector2& v2Size = v2SlotSizes[i];
			const float fY = fSlotY - (fMaxSlotH - v2Size.y) * 0.5f;	// 큰 아이콘 기준 세로 중앙정렬

			eSlotIconTypes[i] = player.GetWeaponInSlot(i);
			pSlotImages[i] = std::make_shared<ImageBox>(GameContext::GetWeaponIconPath(eSlotIconTypes[i]));
			pSlotImages[i]->SetLayer(0);
			pSlotImages[i]->SetAnchor(Vector2{ 1, 1 });
			pSlotImages[i]->SetPivot(Vector2{ 1, 1 });
			pSlotImages[i]->SetPosition(Vector2{ fRight, fY });
			pSlotImages[i]->SetSize(v2Size);
			pUIBoard->InsertUI(pSlotImages[i]);

			// 슬롯 번호 — 아이콘 좌측, 세로 중앙 (아이콘 안쪽에 겹침)
			constexpr float fLabelH = 25.f;
			pSlotLabels[i] = std::make_shared<TextBox>(L"Noto Sans KR");
			pSlotLabels[i]->SetText(std::to_wstring(i + 1));
			pSlotLabels[i]->SetLayer(0);
			pSlotLabels[i]->SetAnchor(Vector2{ 1, 1 });
			pSlotLabels[i]->SetPivot(Vector2{ 1, 1 });
			pSlotLabels[i]->SetPosition(Vector2{ fRight - v2Size.x + 30.f, fY - (v2Size.y - fLabelH) * 0.5f });
			pSlotLabels[i]->SetTextHeight(fLabelH);
			pUIBoard->InsertUI(pSlotLabels[i]);

			fRight -= v2Size.x + fSlotGap;
		}

		// 소모품 슬롯 UI — 무기 슬롯 위 한 줄, 오른쪽 정렬 (붕대 / 폭발 수류탄 / 디코이 수류탄)
		{
			constexpr const char* strItemIcons[3] = {
				"../Resources/Textures/bandage.png",
				"../Resources/Textures/frag_grenade.png",
				"../Resources/Textures/decoy_grenade.png",
			};
			constexpr float fItemSize = 55.f;
			constexpr float fItemGap = 15.f;
			constexpr float fItemY = -250.f;	// 무기 슬롯 상단(-235) 위
			for (int i = 0; i < 3; ++i) {
				const float fItemRight = -20.f - (2 - i) * (fItemSize + fItemGap);

				pItemImages[i] = std::make_shared<ImageBox>(strItemIcons[i]);
				pItemImages[i]->SetLayer(0);
				pItemImages[i]->SetAnchor(Vector2{ 1, 1 });
				pItemImages[i]->SetPivot(Vector2{ 1, 1 });
				pItemImages[i]->SetPosition(Vector2{ fItemRight, fItemY });
				pItemImages[i]->SetSize(Vector2{ fItemSize, fItemSize });
				pUIBoard->InsertUI(pItemImages[i]);

				// 보유 개수 — 아이콘 우하단 (아이템 시스템 구현 후 갱신 연결)
				pItemCounts[i] = std::make_shared<TextBox>(L"Noto Sans KR");
				pItemCounts[i]->SetText(L"0");
				pItemCounts[i]->SetLayer(0);
				pItemCounts[i]->SetAnchor(Vector2{ 1, 1 });
				pItemCounts[i]->SetPivot(Vector2{ 1, 1 });
				pItemCounts[i]->SetPosition(Vector2{ fItemRight - 2.f, fItemY - 2.f });
				pItemCounts[i]->SetTextHeight(20);
				pUIBoard->InsertUI(pItemCounts[i]);
			}
		}

		pReloadAlert = std::make_shared<TextBox>(L"Noto Sans KR");
		pReloadAlert->SetText(L"2");
		pReloadAlert->SetLayer(0);
		pReloadAlert->SetAnchor(Vector2{ 0.5, 0.5 });
		pReloadAlert->SetPivot(Vector2{ 0,0.5 });
		pReloadAlert->SetPosition(Vector2{ 100, 0 });
		pReloadAlert->SetTextHeight(50);
		pReloadAlert->SetVisible(false);
		pUIBoard->InsertUI(pReloadAlert);

		// 붕대 캐스트 라디얼 게이지 — 화면 정중앙, 남은 시간만큼 아이콘이 원형으로 줄어듦
		pBandageCastGauge = std::make_shared<ImageBox>("../Resources/Textures/bandage.png");
		pBandageCastGauge->SetLayer(0);
		pBandageCastGauge->SetAnchor(Vector2{ 0.5, 0.5 });
		pBandageCastGauge->SetPivot(Vector2{ 0.5, 0.5 });
		pBandageCastGauge->SetPosition(Vector2{ 0, 0 });
		pBandageCastGauge->SetSize(Vector2{ 80.f, 80.f });
		pBandageCastGauge->SetVisible(false);
		pUIBoard->InsertUI(pBandageCastGauge);

		// 게이지 바로 아래 퍼센트 텍스트
		pBandageCastText = std::make_shared<TextBox>(L"Noto Sans KR");
		pBandageCastText->SetText(L"");
		pBandageCastText->SetLayer(0);
		pBandageCastText->SetAnchor(Vector2{ 0.5, 0.5 });
		pBandageCastText->SetPivot(Vector2{ 0.5, 0.5 });
		pBandageCastText->SetPosition(Vector2{ 0, 65 });
		pBandageCastText->SetTextHeight(30);
		pBandageCastText->SetVisible(false);
		pUIBoard->InsertUI(pBandageCastText);

		// 붕대 사용법 안내 — 화면 하단 중앙, 들기 모드 중에만 표시
		pBandageHelpText = std::make_shared<TextBox>(L"Noto Sans KR");
		pBandageHelpText->SetText(L"좌클릭 : 자기 회복   우클릭 : 근처 아군 회복");
		pBandageHelpText->SetLayer(0);
		pBandageHelpText->SetAnchor(Vector2{ 0.5, 1 });
		pBandageHelpText->SetPivot(Vector2{ 0.5, 1 });
		pBandageHelpText->SetPosition(Vector2{ 0, -150 });
		pBandageHelpText->SetTextHeight(28);
		pBandageHelpText->SetVisible(false);
		pUIBoard->InsertUI(pBandageHelpText);
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

	if (pSlotImages[0] && player.HasWeaponSlots()) {
		const int nCurSlot = player.GetCurrentWeaponSlot();
		const Vector4 v4Selected{ 1.f, 1.f, 1.f, 1.f };
		const Vector4 v4Dimmed{ 0.45f, 0.45f, 0.45f, 0.8f };

		for (int i = 0; i < 3; ++i) {
			const WEAPON_TYPE eSlotWeapon = player.GetWeaponInSlot(i);
			if (eSlotIconTypes[i] != eSlotWeapon) {
				eSlotIconTypes[i] = eSlotWeapon;
				pSlotImages[i]->SetTexture(GameContext::GetWeaponIconPath(eSlotWeapon));
			}
			// 현재 슬롯만 밝게, 나머지 어둡게
			pSlotImages[i]->SetColor(i == nCurSlot ? v4Selected : v4Dimmed);
			if (pSlotLabels[i]) pSlotLabels[i]->SetColor(i == nCurSlot ? v4Selected : v4Dimmed);
		}
	}

	// 붕대 캐스트 라디얼 게이지 + 퍼센트 (취소/완료 시 m_bInBandage=false → 자동 숨김)
	if (pBandageCastGauge) {
		const bool bCasting = player.IsBandaging();
		if (bCasting) {
			pBandageCastGauge->SetRadialProgress(1.f - player.GetBandageProgress());	// 남은 비율만큼 표시 → 점점 줄어듦
		}
		pBandageCastGauge->SetVisible(bCasting);

		if (pBandageCastText) {
			if (bCasting) {
				const int nPercent = static_cast<int>(player.GetBandageProgress() * 100.f);
				pBandageCastText->SetText(std::format(L"{}%", nPercent));
			}
			pBandageCastText->SetVisible(bCasting);
		}

		// 사용법 안내 — 들고 있는 동안만 (캐스트 중엔 게이지가 대신하므로 숨김)
		// 붕대/수류탄 공용 텍스트 박스 — 어느 쪽을 들었는지에 따라 문구 교체
		if (pBandageHelpText) {
			if (player.IsHoldingGrenade()) {
				pBandageHelpText->SetText(player.IsDecoySelected()
					? L"좌클릭 꾹 : 조준(궤도 표시)   놓으면 : 던지기  [디코이 - 좀비 유인]"
					: L"좌클릭 꾹 : 조준(궤도 표시)   놓으면 : 던지기");
				pBandageHelpText->SetVisible(true);
			}
			else {
				pBandageHelpText->SetText(L"좌클릭 : 자기 회복   우클릭 : 근처 아군 회복");
				pBandageHelpText->SetVisible(player.IsHoldingBandage() && !bCasting);
			}
		}
	}

	// 소모품 슬롯 0번 = 붕대, 1번 = 프래그 수류탄, 2번 = 디코이 수류탄 — 보유 개수 + 들기 모드 하이라이트
	if (pItemCounts[0]) {
		pItemCounts[0]->SetText(std::to_wstring(player.GetBandageCount()));
	}
	if (pItemCounts[1]) {
		pItemCounts[1]->SetText(std::to_wstring(player.GetGrenadeCount()));
	}
	if (pItemCounts[2]) {
		pItemCounts[2]->SetText(std::to_wstring(player.GetDecoyCount()));
	}
	{
		const Vector4 v4Selected{ 1.f, 1.f, 1.f, 1.f };
		const Vector4 v4Dimmed{ 0.45f, 0.45f, 0.45f, 0.8f };
		if (pItemImages[0]) {
			pItemImages[0]->SetColor(player.IsHoldingBandage() ? v4Selected : v4Dimmed);
		}
		if (pItemImages[1]) {
			pItemImages[1]->SetColor(player.IsHoldingGrenade() && !player.IsDecoySelected() ? v4Selected : v4Dimmed);
		}
		if (pItemImages[2]) {
			pItemImages[2]->SetColor(player.IsHoldingGrenade() && player.IsDecoySelected() ? v4Selected : v4Dimmed);
		}
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

	// 붕대 캐스트 진행 — 로컬은 완료 시 회복 적용/전송, 리모트는 같은 타이머로 모션 자체 종료
	if (m_bInBandage) {
		if (IsDead()) {
			CancelBandage();
		}
		else {
			m_fBandageTimer += DT;
			if (m_fBandageTimer >= BANDAGE_CAST_SECONDS) {
				FinishBandage();
			}
		}
	}

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

void IThirdPersonPlayer::SetWeaponSlots(WEAPON_TYPE eWeapon1, WEAPON_TYPE eWeapon2)
{
	m_eWeaponSlots = { eWeapon1, eWeapon2, WEAPON_TYPE::PISTOL };

	// 슬롯별 인스턴스 1회 생성 — 이후 전환은 이 인스턴스를 재사용해 탄약 유지
	auto pOwner = static_pointer_cast<IThirdPersonPlayer>(shared_from_this());
	for (int i = 0; i < 3; ++i) {
		m_pSlotWeapons[i] = static_pointer_cast<WeaponObject>(GCTX->GetWeaponCopy(m_eWeaponSlots[i]));
		m_pSlotWeapons[i]->SetOwner(pOwner);
	}

	m_nCurrentWeaponSlot = 0;
	GiveWeapon(m_eWeaponSlots[0]);
}

void IThirdPersonPlayer::SelectWeaponSlot(int nSlot)
{
	if (nSlot < 0 || nSlot >= 3 || nSlot == m_nCurrentWeaponSlot) return;
	if (!m_pSlotWeapons[nSlot]) return;			// 슬롯 미설정 (리모트 등)
	if (m_bInMeleeAttack || m_bInWeaponSwap || m_bInBandage) return;
	if (auto pWeapon = GetCurrentWeaponObject(); pWeapon && pWeapon->IsInReloading()) return;

	m_nCurrentWeaponSlot = nSlot;
	GiveWeapon(m_eWeaponSlots[nSlot]);
	PlayWeaponDrawAction();
}

void IThirdPersonPlayer::PlayWeaponDrawAction()
{
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	m_bInWeaponSwap = true;

	// 현재 장착된(= 방금 교체된) 무기 기준으로 꺼내기 모션 선택
	if (m_pWeaponSocket->GetCurrentWeaponType() == WEAPON_TYPE::PISTOL) {
		pAnimationCtrl->GetMontage()->PlayMontage("Pistol Draw");
	}
	else {
		pAnimationCtrl->GetMontage()->PlayMontage("Rifle Draw");
	}
}

void IThirdPersonPlayer::OnWeaponDrawEnd()
{
	m_bInWeaponSwap = false;

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());

	// 조준 유지 중이면 새 무기의 조준 idle로 복귀, 아니면 상태머신으로 블렌드 아웃
	if (m_bAiming) {
		if (m_pWeaponSocket->GetCurrentWeaponType() == WEAPON_TYPE::PISTOL) {
			pAnimationCtrl->GetMontage()->JumpToSection("Pistol Aiming Idle");
		}
		else {
			pAnimationCtrl->GetMontage()->JumpToSection("Rifle Aiming Idle");
		}
	}
	else {
		pAnimationCtrl->GetMontage()->StopMontage();
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

	// 총 조준 또는 수류탄 와인드업(좌클릭 홀드) 중엔 캐릭터가 카메라 시선을 따라 회전
	if ((m_bAiming || m_bGrenadeWindup) && m_pCamera) {
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
	// 그림자 패스도 이 큐를 쓰므로 표시 상태를 따라야 함 — 붕대/수류탄 들기 중엔 총 그림자 제외
	if (m_pWeaponSocket && m_pWeaponSocket->IsModelVisible()) {
		if (auto p = m_pWeaponSocket->GetWeaponModel()) {
			p->AddToQueue(pRenderQueue);
		}
	}

	if (m_pGrenadeSocket && m_pGrenadeSocket->IsVisible()) {
		if (auto p = m_pGrenadeSocket->GetModel()) {
			p->AddToQueue(pRenderQueue);
		}
	}

	if (m_pBandageSocket && m_pBandageSocket->IsVisible()) {
		if (auto p = m_pBandageSocket->GetModel()) {
			p->AddToQueue(pRenderQueue);
		}
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

void IThirdPersonPlayer::SetMouseLookEnabled(bool bEnable)
{
	if (m_bMouseInUse != bEnable) {
		ToggleMouseLook();
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
		250_m,
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

	// 붕대 캐스트 중 이동 입력 → 취소.
	// ProcessLocalMovementInput 직후라 m_bMoved는 이번 프레임 키 입력만 반영
	// (낙하로 인한 m_bMoved 갱신은 PostUpdate라 여기 안 섞임). 로컬 전용 경로.
	if (m_bInBandage && m_bMoved) {
		CancelBandage();
	}

	// Gun Handling
	if (m_bMouseInUse) {
		if (m_bHoldingBandage) {
			// 붕대 모드: 좌클릭 = 자기 회복, 우클릭 = 근처 아군 회복 (조준/발사 대신)
			// 대상 해석은 씬(GameScene::ProcessPlayerBandage)이 담당 — 여기선 요청 플래그만 세운다.
			if (!m_bInBandage && !m_bInMeleeAttack && !m_bInWeaponSwap && m_nBandageCount > 0) {
				if (INPUT->GetButtonDown(VK_LBUTTON)) {
					m_nBandageRequest = 1;
				}
				else if (INPUT->GetButtonDown(VK_RBUTTON)) {
					m_nBandageRequest = 2;
				}
			}
		}
		else if (m_bHoldingGrenade) {
			// 수류탄 모드: 좌클릭 꾹 = 와인드업(뒤로 들기), 뗌 = 던지기
			if (!m_bInGrenadeThrow && !m_bInMeleeAttack && !m_bInWeaponSwap
				&& (m_bDecoySelected ? m_nDecoyCount : m_nGrenadeCount) > 0) {
				if (INPUT->GetButtonDown(VK_LBUTTON)) {
					StartGrenadeWindup();
				}
				else if (INPUT->GetButtonUp(VK_LBUTTON) && m_bGrenadeWindup) {
					StartGrenadeThrow();
				}
			}
		}
		else {
			// Aim
			if (INPUT->GetButtonDown(VK_RBUTTON)) {
				EnterAim();
			}

			if (INPUT->GetButtonUp(VK_RBUTTON)) {
				LeaveAim();
			}

			if ((INPUT->GetButtonDown(VK_LBUTTON) || INPUT->GetButtonPressed(VK_LBUTTON)) && m_bAiming) {
				if (!m_bInMeleeAttack && !m_bInWeaponSwap) {
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

	}

	// Fire debug

	if (INPUT->GetButtonDown(VK_RSHIFT)) {
		EnterAim();
	}

	if (INPUT->GetButtonUp(VK_RSHIFT)) {
		LeaveAim();
	}

	if ((INPUT->GetButtonDown(VK_LCONTROL) || INPUT->GetButtonPressed(VK_LCONTROL)) && m_bAiming) {
		if (!m_bInMeleeAttack && !m_bInWeaponSwap) {
			m_bFiredThisFrame = m_pWeaponSocket->TryFire();
			if (m_bFiredThisFrame) {
				PlayFireAction();
			}
		}
	}

	// 무기 슬롯 전환 — 1/2번 = 로비 선택 주무기, 3번 = 권총 고정.
	// 붕대/수류탄 모드면 해제 + 무기 꺼내는 모션까지 (같은 슬롯 복귀 포함).
	// Exit 함수는 자기 모드가 아니면 SelectWeaponSlot으로 위임하므로 수류탄 → 붕대 순서로 체인.
	auto ExitItemToSlot = [this](int nSlot) {
		if (m_bHoldingGrenade) ExitGrenadeToSlot(nSlot);
		else                   ExitBandageToSlot(nSlot);
	};
	if (INPUT->GetButtonDown('1')) ExitItemToSlot(0);
	if (INPUT->GetButtonDown('2')) ExitItemToSlot(1);
	if (INPUT->GetButtonDown('3')) ExitItemToSlot(2);

	// 붕대 들기 — 소모품 슬롯 0번
	if (INPUT->GetButtonDown('4')) {
		EnterBandageMode();
	}

	// 수류탄 들기 — 소모품 슬롯 1번(프래그) / 2번(디코이)
	if (INPUT->GetButtonDown('5')) {
		EnterGrenadeMode(false);
	}
	if (INPUT->GetButtonDown('6')) {
		EnterGrenadeMode(true);
	}

	//  Melee attack
	if (INPUT->GetButtonDown('V') && !m_bInMeleeAttack && !m_bInWeaponSwap && !m_bHoldingBandage && !m_bHoldingGrenade) {
		PlayMeleeStartAction();
	}

	// Reloading
	if (INPUT->GetButtonDown('R') && !m_bInWeaponSwap && !m_bHoldingBandage && !m_bHoldingGrenade) {
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

	if (m_bMoved && !m_bAiming && !m_bGrenadeWindup) {	// 와인드업 중엔 카메라 추종 회전이 우선
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

	// 교체(Weapon Draw) 몽타주 재생 중엔 aim idle 재생을 보류 —
	// 여기서 끊으면 종료 notify가 누락되어 m_bInWeaponSwap 이 영구히 true 로 남는다.
	// OnWeaponDrawEnd 가 m_bAiming 을 보고 aim idle로 복귀시킨다.
	// 전신(사망) 몽타주 재생 중엔 aim idle로 덮지 않는다 — 리모트는 transform 패킷의
	// bAiming으로 매번 EnterAim이 불려 시체가 조준 자세로 벌떡 일어나는 문제 방지.
	auto* pMontage = pAnimationCtrl->GetMontage().get();
	const bool bFullBodyMontage = pMontage->IsPlaying() && pMontage->IsCurrentSectionFullBody();
	if (GetCurrentWeaponObject()->IsInReloading() == false && !m_bInWeaponSwap && !bFullBodyMontage) {
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
	// 전신(사망) 몽타주는 여기서 끊지 않는다 — 조준 중 사망 시 다음 프레임
	// ProcessInput의 IsDead→LeaveAim이 사망 몽타주를 블렌드아웃시켜
	// 시체가 서있는 채 멈춰 보이는 문제의 원인이었음.
	auto* pMontage = pAnimationCtrl->GetMontage().get();
	const bool bFullBodyMontage = pMontage->IsPlaying() && pMontage->IsCurrentSectionFullBody();
	if (!GetCurrentWeaponObject()->IsInReloading() && !m_bInMeleeAttack && !m_bInWeaponSwap && !bFullBodyMontage) {
		pMontage->StopMontage();
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

	// 리모트: 발사 이벤트가 Weapon Draw 몽타주를 끊으면 종료 notify가 안 오므로
	// 여기서 잠금을 푼다 (무기 자체는 draw 시작 시 이미 교체 완료 상태).
	m_bInWeaponSwap = false;

	auto pAnimationCtrl = std::static_pointer_cast<PlayerAnimationController>(GetComponent<AnimationController>());

	if (m_pWeaponSocket->GetCurrentWeaponType() != WEAPON_TYPE::PISTOL) {
		pAnimationCtrl->GetMontage()->JumpToSection("Rifle Fire");
	}
	else {
		pAnimationCtrl->GetMontage()->JumpToSection("Pistol Fire");
	}

	auto pWeapon = m_pWeaponSocket->GetWeaponModel();
	if (pWeapon) {
		if (m_PlayerHUD.pCrosshair) {
			m_PlayerHUD.pCrosshair->AddRecoil(pWeapon->GetRecoil());
		}

		if (m_pCamera) {
			auto pThirdPersonCamera = std::static_pointer_cast<ThirdPersonCamera>(m_pCamera);
			pThirdPersonCamera->AddFireFovRecoil(pWeapon->GetRecoil(), pWeapon->GetRecoilRecovery());
		}
	}

	//if (m_PlayerHUD.pHitMarker && m_pWeaponSocket->GetWeaponModel()) {
	//	m_PlayerHUD.pHitMarker->Show();
	//}

}

void IThirdPersonPlayer::PlayMeleeStartAction()
{
	// 무기 교체 몽타주를 끊으면 종료 notify 누락으로 m_bInWeaponSwap 이 안 풀린다
	if (m_bInMeleeAttack || m_bInWeaponSwap) {
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
	if (m_bInWeaponSwap) return;	// 교체 몽타주 보호 (종료 notify 누락 방지)

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

void IThirdPersonPlayer::EnterBandageMode()
{
	if (m_bHoldingBandage || m_bInBandage || m_bInGrenadeThrow || m_bInMeleeAttack || m_bInWeaponSwap) return;
	if (auto pWeapon = GetCurrentWeaponObject(); pWeapon && pWeapon->IsInReloading()) return;
	if (m_nBandageCount <= 0) return;

	// 수류탄 들기 중이면 내려놓고 바로 붕대로 — 총 꺼내기(ExitGrenadeToSlot) 없이 직행
	if (m_bHoldingGrenade) {
		m_bHoldingGrenade = false;
		if (m_bGrenadeWindup && m_pCamera) {
			// 와인드업 중 전환 — 줌 복귀
			std::static_pointer_cast<ThirdPersonCamera>(m_pCamera)->LeaveAimMode();
		}
		m_bGrenadeWindup = false;
		SetGrenadeHandVisible(false);
		if (GetCamera()) {
			NetworkManager::GetInstance()->SendPlayerGrenade(4, Vector3::Zero, Vector3::Zero);
		}
	}

	LeaveAim();		// 멱등 — 비조준이면 no-op
	m_bHoldingBandage = true;

	// 붕대 들기 = 총을 내림 (장착/탄약 상태는 유지, 렌더만 끔) + 손에 붕대 표시
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(false);
	}
	SetBandageHandVisible(true);

	// 붕대 꺼내는 모션 — 무기 드로우와 같은 잠금(m_bInWeaponSwap) 공유, 종료 notify(OnWeaponDrawEnd)가 해제
	m_bInWeaponSwap = true;
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");

	// 리모트 들기 모션 동기화
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerBandage(3, -1);
	}
}

void IThirdPersonPlayer::ExitBandageToSlot(int nSlot)
{
	if (!m_bHoldingBandage) {
		SelectWeaponSlot(nSlot);
		return;
	}

	if (m_bInBandage || m_bInWeaponSwap) return;	// 캐스트/드로우 중 잠금 (몽타주/상태 보호)

	m_bHoldingBandage = false;
	SetBandageHandVisible(false);
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	// 다른 슬롯이면 무기 교체까지, 같은 슬롯이면 그대로 — 어느 쪽이든 꺼내는 모션 재생
	if (nSlot != m_nCurrentWeaponSlot && nSlot >= 0 && nSlot < 3 && m_pSlotWeapons[nSlot]) {
		m_nCurrentWeaponSlot = nSlot;
		GiveWeapon(m_eWeaponSlots[nSlot]);
	}
	PlayWeaponDrawAction();

	// 리모트 내리기 모션 동기화 (무기 교체는 GiveWeapon이 이미 전송)
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerBandage(4, -1);
	}
}

void IThirdPersonPlayer::PlayBandageHoldAction()
{
	// 리모트 전용: 총 내림 + 붕대 꺼내는 모션 (로컬 EnterBandageMode와 동일한 손 연출)
	if (m_bHoldingBandage || m_bInBandage || m_bInMeleeAttack) return;

	m_bHoldingBandage = true;	// 캐스트 종료(StopBandageAction) 후에도 총 숨김 유지 기준

	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(false);
	}
	SetBandageHandVisible(true);

	m_bInWeaponSwap = true;
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");
}

void IThirdPersonPlayer::PlayBandageUnholdAction()
{
	if (!m_bHoldingBandage) return;

	m_bHoldingBandage = false;
	SetBandageHandVisible(false);
	StopBandageAction();	// 캐스트 중이었다면 모션도 정리 (멱등)

	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	PlayWeaponDrawAction();	// 총 다시 꺼내는 모션
}

void IThirdPersonPlayer::PlayBandageStartAction(bool bAllyTarget)
{
	if (m_bInBandage || m_bInMeleeAttack || m_bInWeaponSwap) return;

	m_bInBandage = true;
	m_fBandageTimer = 0.f;

	// 리모트: 감는 동안 총 내림 (로컬은 들기 모드에서 이미 숨김 — 멱등)
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(false);
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage(bAllyTarget ? "Bandage Ally Wrap" : "Bandage Wrap");
}

void IThirdPersonPlayer::StopBandageAction()
{
	if (!m_bInBandage) return;

	m_bInBandage = false;
	m_fBandageTimer = 0.f;

	// 들기 모드가 아니면(리모트/취소 후 무기 복귀) 총 다시 표시.
	// 로컬 들기 모드 중엔 숨김 유지 — LeaveBandageMode에서 복원.
	if (m_pWeaponSocket && !m_bHoldingBandage) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->StopMontage();
}

void IThirdPersonPlayer::StartBandageCast(int targetPlayerId)
{
	if (m_bInBandage || m_bInMeleeAttack || m_bInWeaponSwap) return;

	m_nBandageTargetId = targetPlayerId;

	// 자기(-1 = 오프라인 자기, 본인 id = 온라인 자기) / 아군(다른 id) 모션 구분
	const bool bAllyTarget = targetPlayerId >= 0
		&& targetPlayerId != NetworkManager::GetInstance()->GetPlayerID();
	PlayBandageStartAction(bAllyTarget);

	// 로컬 플레이어(카메라 보유)만 서버에 시작 알림 — 리모트 붕대 모션 동기화
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerBandage(0, targetPlayerId);
	}
}

void IThirdPersonPlayer::FinishBandage()
{
	const int nTargetId = m_nBandageTargetId;
	StopBandageAction();
	m_nBandageTargetId = -1;

	// 리모트: 모션 종료만 (회복은 S2C_PLAYER_HEAL로 대상 클라가 반영)
	if (!GetCamera()) return;

	m_nBandageCount = std::max(0, m_nBandageCount - 1);

	auto pNetwork = NetworkManager::GetInstance();
	if (pNetwork->IsConnected() && !pNetwork->IsOffline()) {
		pNetwork->SendPlayerBandage(2, nTargetId);	// 서버가 회복 적용 + S2C_PLAYER_HEAL 확정
	}
	else {
		Heal(BANDAGE_HEAL_AMOUNT);	// 오프라인은 자기 자신만
	}
}

void IThirdPersonPlayer::CancelBandage()
{
	if (!m_bInBandage) return;

	const int nTargetId = m_nBandageTargetId;
	StopBandageAction();
	m_nBandageTargetId = -1;

	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerBandage(1, nTargetId);
	}
}

void IThirdPersonPlayer::SetGrenadeHandVisible(bool bVisible)
{
	if (bVisible && !m_pGrenadeSocket) {
		auto pSkeleton = GetComponent<Skeleton>();
		if (!pSkeleton) return;
		m_pGrenadeSocket = pSkeleton->CreateAttachSocket<GrenadeHandSocket>("RightHand"s);
		if (m_pGrenadeSocket) {
			m_pGrenadeSocket->Initialize();	// 런타임 생성 — Skeleton::Initialize 이후라 직접 초기화
		}
	}

	if (m_pGrenadeSocket) {
		if (bVisible) {
			m_pGrenadeSocket->SetDecoy(m_bDecoySelected);	// 일반(granade) ↔ 디코이(stun_grenade) 손 모델
		}
		m_pGrenadeSocket->SetVisible(bVisible);
	}
}

void IThirdPersonPlayer::SetBandageHandVisible(bool bVisible)
{
	if (bVisible && !m_pBandageSocket) {
		auto pSkeleton = GetComponent<Skeleton>();
		if (!pSkeleton) return;
		m_pBandageSocket = pSkeleton->CreateAttachSocket<BandageHandSocket>("RightHand"s);
		if (m_pBandageSocket) {
			m_pBandageSocket->Initialize();	// 런타임 생성 — Skeleton::Initialize 이후라 직접 초기화
		}
	}

	if (m_pBandageSocket) {
		m_pBandageSocket->SetVisible(bVisible);
	}
}

void IThirdPersonPlayer::EnterGrenadeMode(bool bDecoy)
{
	// 이미 들기 모드면 종류 전환 — 와인드업/던지기 중엔 잠금 (모션·투사체 종류 불일치 방지)
	if (m_bHoldingGrenade) {
		if (m_bDecoySelected == bDecoy) return;
		if (m_bGrenadeWindup || m_bInGrenadeThrow || m_bInWeaponSwap) return;
		if ((bDecoy ? m_nDecoyCount : m_nGrenadeCount) <= 0) return;
		m_bDecoySelected = bDecoy;
		SetGrenadeHandVisible(true);	// 손 모델을 새 종류로 교체 (granade ↔ stun_grenade)

		// 종류 전환도 꺼내는 모션 재생 — 첫 진입과 동일한 드로우 잠금 공유 (OnWeaponDrawEnd가 해제)
		m_bInWeaponSwap = true;
		auto pAnimationCtrl =
			std::static_pointer_cast<PlayerAnimationController>(
				GetComponent<AnimationController>());
		pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");

		// 리모트도 들기 모션 다시 재생 + 손 모델 종류 동기화
		if (GetCamera()) {
			NetworkManager::GetInstance()->SendPlayerGrenade(3, Vector3::Zero, Vector3::Zero, bDecoy ? 1 : 0);
		}
		return;
	}

	if (m_bInBandage || m_bInMeleeAttack || m_bInWeaponSwap) return;
	if (auto pWeapon = GetCurrentWeaponObject(); pWeapon && pWeapon->IsInReloading()) return;
	if ((bDecoy ? m_nDecoyCount : m_nGrenadeCount) <= 0) return;
	m_bDecoySelected = bDecoy;

	// 붕대 들기 중이면 내려놓고 바로 수류탄으로 — 총 꺼내기(ExitBandageToSlot) 없이 직행
	if (m_bHoldingBandage) {
		m_bHoldingBandage = false;
		SetBandageHandVisible(false);
		if (GetCamera()) {
			NetworkManager::GetInstance()->SendPlayerBandage(4, -1);
		}
	}

	LeaveAim();		// 멱등 — 비조준이면 no-op
	m_bHoldingGrenade = true;

	// 수류탄 들기 = 총을 내림 (장착/탄약 상태는 유지, 렌더만 끔) + 손에 수류탄 표시
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(false);
	}
	SetGrenadeHandVisible(true);

	// 꺼내는 모션 — 붕대와 동일하게 드로우 잠금(m_bInWeaponSwap) 공유, 종료 notify(OnWeaponDrawEnd)가 해제
	m_bInWeaponSwap = true;
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");

	// 리모트 들기 모션 동기화 (grenadeType으로 손 모델 종류도 전달)
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerGrenade(3, Vector3::Zero, Vector3::Zero, bDecoy ? 1 : 0);
	}
}

void IThirdPersonPlayer::ExitGrenadeToSlot(int nSlot)
{
	if (!m_bHoldingGrenade) {
		SelectWeaponSlot(nSlot);
		return;
	}

	if (m_bInGrenadeThrow || m_bInWeaponSwap) return;	// 던지기/드로우 중 잠금 (몽타주/상태 보호)

	m_bHoldingGrenade = false;
	if (m_bGrenadeWindup && m_pCamera) {
		// 와인드업 중 슬롯 전환 — 줌 복귀
		std::static_pointer_cast<ThirdPersonCamera>(m_pCamera)->LeaveAimMode();
	}
	m_bGrenadeWindup  = false;	// 와인드업 중이었다면 취소 (몽타주는 무기 드로우가 대체)
	SetGrenadeHandVisible(false);
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	// 다른 슬롯이면 무기 교체까지, 같은 슬롯이면 그대로 — 어느 쪽이든 꺼내는 모션 재생
	if (nSlot != m_nCurrentWeaponSlot && nSlot >= 0 && nSlot < 3 && m_pSlotWeapons[nSlot]) {
		m_nCurrentWeaponSlot = nSlot;
		GiveWeapon(m_eWeaponSlots[nSlot]);
	}
	PlayWeaponDrawAction();

	// 리모트 내리기 모션 동기화 (무기 교체는 GiveWeapon이 이미 전송)
	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerGrenade(4, Vector3::Zero, Vector3::Zero);
	}
}

void IThirdPersonPlayer::StartGrenadeWindup()
{
	if (m_bGrenadeWindup || m_bInGrenadeThrow || m_bInMeleeAttack || m_bInWeaponSwap) return;

	m_bGrenadeWindup = true;

	// 총 조준처럼 카메라만 줌인 (m_bAiming은 무기 로직과 얽혀 있어 건드리지 않음)
	if (m_pCamera) {
		std::static_pointer_cast<ThirdPersonCamera>(m_pCamera)->EnterAimMode();
	}

	// 원본 Throw 클립 재생 → 뒤로 든 자세(HOLD_RATIO)에서 일시정지, 좌클릭 뗌까지 유지
	const auto& pMontage =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>())->GetMontage();
	pMontage->PlayMontage("Grenade Throw");
	pMontage->PauseAtRatio(GRENADE_HOLD_RATIO);

	if (GetCamera()) {
		NetworkManager::GetInstance()->SendPlayerGrenade(2, Vector3::Zero, Vector3::Zero);
	}
}

void IThirdPersonPlayer::StartGrenadeThrow()
{
	if (!m_bGrenadeWindup || m_bInGrenadeThrow) return;

	m_bGrenadeWindup  = false;
	m_bInGrenadeThrow = true;

	// 와인드업 줌 복귀
	if (m_pCamera) {
		std::static_pointer_cast<ThirdPersonCamera>(m_pCamera)->LeaveAimMode();
	}

	// 일시정지 해제 — 홀드 지점부터 이어서 재생 (릴리즈/종료 notify 정상 발화)
	const auto& pMontage =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>())->GetMontage();
	if (pMontage->IsPausedHold()) {
		pMontage->ResumeMontage();
	}
	else {
		pMontage->PlayMontage("Grenade Throw");	// 홀드 몽타주가 끊긴 예외 상황 폴백
	}

	// 투척 패킷(state 0)은 릴리즈 notify 시점에 씬이 투사체 스폰과 함께 전송
}

void IThirdPersonPlayer::OnGrenadeRelease()
{
	// 손에서 투사체로 교체 — 리모트/로컬 공통
	SetGrenadeHandVisible(false);

	// 리모트 플레이어도 몽타주 notify가 불리지만, 투사체 스폰은 S2C 패킷(state 0)이
	// 담당하므로 로컬(카메라 보유)만 요청 플래그를 세운다.
	if (!GetCamera()) return;

	m_bGrenadeReleasePending = true;
	if (m_bDecoySelected)
		m_nDecoyCount = std::max(0, m_nDecoyCount - 1);
	else
		m_nGrenadeCount = std::max(0, m_nGrenadeCount - 1);
}

void IThirdPersonPlayer::OnGrenadeThrowEnd()
{
	m_bInGrenadeThrow = false;

	// 로컬: 든 종류의 수류탄이 다 떨어지면 현재 슬롯 무기로 자동 복귀 (리모트는 state 4 패킷이 처리)
	if (GetCamera() && m_bHoldingGrenade
		&& (m_bDecoySelected ? m_nDecoyCount : m_nGrenadeCount) <= 0) {
		ExitGrenadeToSlot(m_nCurrentWeaponSlot);
		return;
	}

	// 계속 들기 모드면 다음 투척용 수류탄을 다시 손에 표시
	if (m_bHoldingGrenade) {
		SetGrenadeHandVisible(true);
	}
}

void IThirdPersonPlayer::PlayGrenadeEquipAction(bool bDecoy)
{
	// 리모트 전용: 총 내림 + 꺼내는 모션 (로컬 EnterGrenadeMode와 동일한 손 연출)
	if (m_bInMeleeAttack) return;

	// 이미 들기 중 = 종류 전환(5↔6) — 손 모델 교체 + 드로우 모션 재생. 드로우 진행 중이면
	// 모션만 스킵: 몽타주를 대체하면 OnWeaponDrawEnd notify가 누락돼 m_bInWeaponSwap이 안 풀린다.
	if (m_bHoldingGrenade) {
		m_bDecoySelected = bDecoy;
		SetGrenadeHandVisible(true);	// 손 모델을 새 종류로 교체
		if (m_bInWeaponSwap) return;
		m_bInWeaponSwap = true;
		auto pAnimationCtrl =
			std::static_pointer_cast<PlayerAnimationController>(
				GetComponent<AnimationController>());
		pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");
		return;
	}

	m_bHoldingGrenade = true;
	m_bDecoySelected  = bDecoy;	// 손 모델 종류 (SetGrenadeHandVisible가 참조)

	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(false);
	}
	SetGrenadeHandVisible(true);

	m_bInWeaponSwap = true;
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage("Bandage Draw");
}

void IThirdPersonPlayer::PlayGrenadeUnequipAction()
{
	if (!m_bHoldingGrenade) return;

	m_bHoldingGrenade = false;
	m_bGrenadeWindup  = false;
	m_bInGrenadeThrow = false;
	SetGrenadeHandVisible(false);

	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	PlayWeaponDrawAction();	// 총 다시 꺼내는 모션
}

void IThirdPersonPlayer::PlayGrenadeWindupAction()
{
	// 리모트 전용. 드로우 몽타주 진행 중이면 스킵 — 대체하면 OnWeaponDrawEnd notify가
	// 누락돼 m_bInWeaponSwap이 안 풀린다 (붕대 리모트 모션과 동일한 규칙).
	if (m_bInMeleeAttack || m_bInWeaponSwap) return;

	m_bGrenadeWindup = true;
	const auto& pMontage =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>())->GetMontage();
	pMontage->PlayMontage("Grenade Throw");
	pMontage->PauseAtRatio(GRENADE_HOLD_RATIO);
}

void IThirdPersonPlayer::PlayGrenadeThrowAction()
{
	if (m_bInMeleeAttack || m_bInWeaponSwap) return;

	m_bGrenadeWindup  = false;
	m_bInGrenadeThrow = true;
	const auto& pMontage =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>())->GetMontage();
	if (pMontage->IsPausedHold()) {
		pMontage->ResumeMontage();	// 와인드업 홀드 중이면 이어서
	}
	else {
		pMontage->PlayMontage("Grenade Throw");	// 와인드업 패킷을 못 받은 경우 처음부터
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

	// 슬롯 캐시에 있는 무기면 그 인스턴스 재장착 (탄약 유지 — 근접공격 복귀 포함).
	// 현재 슬롯 우선 → 동일 무기를 두 슬롯에 넣어도 현재 슬롯 것이 잡힌다.
	std::shared_ptr<WeaponObject> pCached = nullptr;
	if (m_pSlotWeapons[m_nCurrentWeaponSlot] && m_eWeaponSlots[m_nCurrentWeaponSlot] == eWeaponType) {
		pCached = m_pSlotWeapons[m_nCurrentWeaponSlot];
	}
	else {
		for (int i = 0; i < 3; ++i) {
			if (m_pSlotWeapons[i] && m_eWeaponSlots[i] == eWeaponType) { pCached = m_pSlotWeapons[i]; break; }
		}
	}

	if (pCached)
		m_pWeaponSocket->SetWeaponObject(pCached, eWeaponType);
	else
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
	// 진행 중 액션 정리 — 사망 몽타주가 기존 몽타주를 끊으면 종료 notify가 누락되므로
	// 잠금 플래그를 직접 해제한다 (부활 후 발사/교체가 영구히 잠기는 것 방지)
	CancelBandage();
	m_bHoldingBandage = false;
	SetBandageHandVisible(false);
	SetGrenadeHandVisible(false);
	m_bHoldingGrenade = false;
	m_bGrenadeWindup  = false;
	m_bInGrenadeThrow = false;
	m_bInMeleeAttack  = false;
	m_bInWeaponSwap   = false;
	m_bMoved   = false;
	m_bRunning = false;

	// 조준 상태 정리 — 사망 몽타주 시작 전에 해제해야 다음 프레임 ProcessInput의
	// IsDead→LeaveAim 경로가 아예 안 탄다 (카메라 줌아웃/크로스헤어 off 포함)
	LeaveAim();

	// 시체가 맨손이 되지 않게 총 표시 복원 (붕대/수류탄 모드 중 사망 대비)
	if (m_pWeaponSocket) {
		m_pWeaponSocket->SetModelVisible(true);
	}

	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->PlayMontage("Player Death");
}

void IThirdPersonPlayer::ApplyRespawn()
{
	// FREEZE로 고정된 사망 몽타주 해제 → 블렌드아웃 후 상태머신(idle) 복귀
	auto pAnimationCtrl =
		std::static_pointer_cast<PlayerAnimationController>(
			GetComponent<AnimationController>());
	pAnimationCtrl->GetMontage()->StopMontage();
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
	// 텍스트 입력(채팅 등) 중에는 게임플레이 입력 차단
	if (INPUT->IsTextInputMode())
		return;

	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		ToggleMouseLook();
	}

	// 사망(관전) 중: 시점 회전만 허용 — 이동/사격/근접/재장전 차단
	if (IsDead()) {
		if (m_bAiming) LeaveAim();
		m_bMoved = false;
		m_v3MoveDirection = Vector3::Zero;
		ProcessLocalCameraInput();
		return;
	}

	ProcessLocalCameraInput();
	ProcessLocalMovementInput();
	ProcessLocalActionInput();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// NetworkOwnerThirdPersonPlayer

void NetworkOwnerThirdPersonPlayer::ProcessInput()
{
	// 텍스트 입력(채팅 등) 중에는 게임플레이 입력 차단
	if (INPUT->IsTextInputMode())
		return;

	// 디버그용 마우스 사용/헤제
	if (INPUT->GetButtonDown(VK_OEM_3)) {	// " ` " -> 물결표 그 버튼임
		ToggleMouseLook();
	}

	// 사망(관전) 중: 시점 회전만 허용 — 이동/사격/근접/재장전 차단
	if (IsDead()) {
		if (m_bAiming) LeaveAim();
		m_bMoved = false;
		m_v3MoveDirection = Vector3::Zero;
		ProcessLocalCameraInput();
		return;
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
	static float fLastSendTime = -1.f;

	// 상태 변화 체크 (또는 움직임 체크)
	bool bStateChanged = (bLastAiming != m_bAiming) || (bLastRunning != m_bRunning);

	float fNow = NetworkManager::GetNetTimeSec();

	// 이동/조준 갱신은 30Hz로 스로틀. 매 프레임 전송하면 고프레임에서 4~16ms
	// 간격의 소형 패킷이 되고, TCP 병합으로 리모트에 버스트로 몰려 도착해
	// 도착시각 기반 스냅샷 보간이 요동친다(속도가 0↔정상 깜빡임 → 걷기 애니 미재생).
	bool bMoveTick = (m_bMoved || m_bAiming || m_bGrenadeWindup) && (fNow - fLastSendTime >= 1.f / 30.f);

	// 정지 중에도 100ms 킵얼라이브 전송 — 안 보내면 리모트가 정지를 모른다:
	// 마지막 이동 스냅샷 속도로 외삽해 오버슈트하고, 걷기 애니가 제자리에서 고착됨
	bool bKeepAlive = (fNow - fLastSendTime >= 0.1f);

	if (bMoveTick || bStateChanged || bKeepAlive) {
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
		fLastSendTime = fNow;
	}
}

void NetworkRemoteThirdPersonPlayer::UpdateNetworkTransform(const Matrix& mtxWorld, bool bRunning, bool bAiming, float fAimPitch, float receivedTime)
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

	// ── 위치/회전: 즉시 스냅 대신 스냅샷 축적 → Update()에서 지연 보간 ──────
	TransformSnapshot snap;
	snap.fTime = receivedTime;

	// 스냅샷 시각은 단조 증가해야 보간 구간 탐색이 성립 — 역행 입력은 직전 시각으로 클램프
	if (!m_Snapshots.empty() && snap.fTime < m_Snapshots.back().fTime)
		snap.fTime = m_Snapshots.back().fTime;

	Matrix mtxCopy = mtxWorld;
	if (!mtxCopy.Decompose(snap.v3Scale, snap.qRot, snap.v3Pos)) {
		// 분해 불가능한 비정상 행렬 — 예전처럼 그대로 적용하고 히스토리 리셋
		m_Snapshots.clear();
		pTransform->SetWorldMatrix(mtxWorld);
		return;
	}

	// 패킷 간격 EMA의 1.5배를 보간 딜레이로. 마지막 간격 하나로 정하면
	// 간격 흔들림(프레임 정렬/네트워크 지터)마다 렌더 시간축이 앞뒤로 점프해 위치 팝 발생.
	// 0.5s 초과 갭은 패킷 손실/일시정지 — 정상 전송 간격이 아니므로 EMA에서 제외.
	// 하한은 정지 킵얼라이브 간격(100ms)보다 커야 한다 — 이동 중 EMA(~16ms)로
	// 하한까지 내려간 채 정지하면 렌더 시각이 최신 스냅샷을 추월, 마지막 이동
	// 속도로 외삽해 오버슈트했다가 되돌아오며 걷기 애니가 추가 재생된다.
	if (!m_Snapshots.empty()) {
		float fInterval = snap.fTime - m_Snapshots.back().fTime;
		if (fInterval > 0.01f && fInterval < 0.5f) {
			m_fAvgPacketInterval = (m_fAvgPacketInterval <= 0.f)
				? fInterval
				: m_fAvgPacketInterval + 0.1f * (fInterval - m_fAvgPacketInterval);
			m_fInterpDelay = std::clamp(m_fAvgPacketInterval * 1.5f, 0.12f, 0.25f);
		}
	}

	// 첫 스냅샷이거나 순간이동(리스폰/재배치)이면 보간 없이 즉시 적용
	if (m_Snapshots.empty() ||
		Vector3::DistanceSquared(m_Snapshots.back().v3Pos, snap.v3Pos) > TELEPORT_DIST * TELEPORT_DIST) {
		m_Snapshots.clear();
		m_fSmoothedSpeed = 0.f; // 순간이동 직후 프레임 델타가 걷기로 오인되지 않도록
		pTransform->SetWorldMatrix(mtxWorld);
	}

	// 정지(킵얼라이브만 수신) 후 도착한 첫 이동 스냅샷: 직전 스냅샷 시각을 새
	// 스냅샷 직전으로 당겨 이동 구간을 실제 프레임 길이 수준으로 압축한다.
	// 안 하면 한 프레임 이동량이 킵얼라이브 갭(최대 100ms) 전체에 퍼져,
	// 살짝만 움직여도 리모트 걷기 애니가 그만큼 길게 재생된다.
	// 직전 구간도 이동 중이었다면(감속 슬라이드 중 킵얼라이브 등) 정상 이동
	// 데이터이므로 압축하지 않는다.
	if (m_Snapshots.size() >= 2) {
		auto& back = m_Snapshots.back();
		const auto& prev = m_Snapshots[m_Snapshots.size() - 2];
		const float fStillEpsSq = 1.f; // 1cm² — 정지 판정 (킵얼라이브 위치 지터 허용)
		float fPrevDx = back.v3Pos.x - prev.v3Pos.x;
		float fPrevDz = back.v3Pos.z - prev.v3Pos.z;
		float fCurDx = snap.v3Pos.x - back.v3Pos.x;
		float fCurDz = snap.v3Pos.z - back.v3Pos.z;
		bool bWasStill  = (fPrevDx * fPrevDx + fPrevDz * fPrevDz) < fStillEpsSq;
		bool bNowMoving = (fCurDx * fCurDx + fCurDz * fCurDz) >= fStillEpsSq;
		if (bWasStill && bNowMoving) {
			back.fTime = std::max(back.fTime, snap.fTime - MOVE_START_SEGMENT);
		}
	}

	m_Snapshots.push_back(snap);

	// 정리는 수량이 아니라 시간 기준 — 전송 주기가 어떻든 보간 딜레이(≤0.25s)
	// 구간이 항상 창 안에 남는다. 수량 고정이면 전송이 빨라질 때 창이 딜레이보다
	// 짧아져, 렌더 시점이 항상 최고(最古) 스냅샷 이전에 갇혀 위치가 계단식으로 튄다.
	while (m_Snapshots.size() > 2 && m_Snapshots.front().fTime < snap.fTime - SNAPSHOT_WINDOW)
		m_Snapshots.pop_front();
	while (m_Snapshots.size() > MAX_SNAPSHOTS)
		m_Snapshots.pop_front();

	// 이동 속도/애니메이션 상태는 여기서 계산하지 않는다 — 패킷 델타 기준이면
	// 애니(패킷 시각)와 위치(보간 시각)의 시간축이 어긋나 멈춘 뒤에도 걷기가 남는다.
	// ApplyInterpolatedTransform()에서 실제 적용되는 보간 위치의 프레임 델타로 계산.
	m_fLastPacketTime = TIME->GetTotalTime(); // 패킷 타임아웃 판정용 (SyncSceneWithServer)
}

void NetworkRemoteThirdPersonPlayer::Update()
{
	ApplyInterpolatedTransform();
	IThirdPersonPlayer::Update();
}

// 매 프레임: 도착 시각 기준 m_fInterpDelay만큼 과거 시점의 스냅샷 보간값을 적용.
// 좀비(Zombie::PostUpdate 온라인 분기)와 동일한 시간축/외삽 규칙.
void NetworkRemoteThirdPersonPlayer::ApplyInterpolatedTransform()
{
	if (m_Snapshots.empty())
		return;

	const float fRenderTime = NetworkManager::GetNetTimeSec() - m_fInterpDelay;

	// 기본값: 최신 스냅샷 (스냅샷 1개뿐이거나 구간을 못 찾은 경우)
	Vector3    v3Scale = m_Snapshots.back().v3Scale;
	Quaternion qRot    = m_Snapshots.back().qRot;
	Vector3    v3Pos   = m_Snapshots.back().v3Pos;

	if (fRenderTime <= m_Snapshots.front().fTime) {
		// 렌더 시점이 가장 오래된 스냅샷보다 과거 — 가장 오래된 값 유지
		v3Scale = m_Snapshots.front().v3Scale;
		qRot    = m_Snapshots.front().qRot;
		v3Pos   = m_Snapshots.front().v3Pos;
	}
	else {
		bool bFound = false;
		for (int i = static_cast<int>(m_Snapshots.size()) - 1; i >= 1; --i) {
			const auto& s0 = m_Snapshots[i - 1];
			const auto& s1 = m_Snapshots[i];
			if (s0.fTime <= fRenderTime && fRenderTime <= s1.fTime) {
				float fSpan = s1.fTime - s0.fTime;
				float t = (fSpan > 0.f) ? (fRenderTime - s0.fTime) / fSpan : 1.f;
				v3Pos   = Vector3::Lerp(s0.v3Pos, s1.v3Pos, t);
				qRot    = Quaternion::Slerp(s0.qRot, s1.qRot, t); // 최단경로 회전 보간
				v3Scale = Vector3::Lerp(s0.v3Scale, s1.v3Scale, t);
				bFound = true;
				break;
			}
		}

		// 렌더 시점이 최신 스냅샷보다 미래(패킷 공백) — 위치만 속도 기반 외삽(클램프),
		// 회전/스케일은 최신값 유지
		if (!bFound && m_Snapshots.size() >= 2) {
			const auto& s0 = m_Snapshots[m_Snapshots.size() - 2];
			const auto& s1 = m_Snapshots.back();
			float fSpan = s1.fTime - s0.fTime;
			if (fSpan > 0.f) {
				Vector3 v3Vel = (s1.v3Pos - s0.v3Pos) * (1.f / fSpan);
				float fExtrap = std::min(fRenderTime - s1.fTime, MAX_EXTRAPOLATION);
				v3Pos = s1.v3Pos + v3Vel * fExtrap;
			}
		}
	}

	// ── 애니메이션 속도 = 실제 적용되는 보간 위치의 프레임 델타 ─────────────
	// 화면에 보이는 이동과 애니 상태가 정의상 항상 일치 (좀비와 동일한 철학).
	Vector3 v3DeltaXZ = v3Pos - GetTransform()->GetPosition();
	v3DeltaXZ.y = 0.f;
	float fDistSq = v3DeltaXZ.LengthSquared();
	float fDT = std::max(DT, 0.0001f);
	float fSpeedRaw = std::sqrt(fDistSq) / fDT;

	// 프레임 델타는 패킷 도착 버스트/프레임 정렬로 한두 프레임 0이 될 수 있다.
	// 원값을 그대로 쓰면 m_bMoved가 매 프레임 토글되고, 상태머신은 전이 때마다
	// 블렌드를 재시작하므로 가중치가 0 근처에 머물러 걷기 모션이 아예 안 보인다.
	// 짧은 EMA로 스무딩해 1~2 프레임 공백을 넘긴다 (정지 감지 지연 ~80ms).
	m_fSmoothedSpeed += 0.35f * (fSpeedRaw - m_fSmoothedSpeed);

	if (m_fSmoothedSpeed > 30.f) { // 30cm/s 미만은 보간 잔떨림으로 간주 — 걷기 오발동 방지
		m_bMoved = true;
		if (fDistSq > 1e-4f) { // 공백 프레임엔 직전 이동 방향 유지
			m_v3MoveDirection = v3DeltaXZ;
			m_v3MoveDirection.Normalize();
		}
		m_fMoveSpeed = m_fSmoothedSpeed;
	}
	else {
		m_bMoved = false;
		m_fMoveSpeed = 0.f;
	}

	Matrix mtx = Matrix::CreateScale(v3Scale) * Matrix::CreateFromQuaternion(qRot);
	mtx._41 = v3Pos.x;
	mtx._42 = v3Pos.y;
	mtx._43 = v3Pos.z;
	GetTransform()->SetWorldMatrix(mtx);
}
