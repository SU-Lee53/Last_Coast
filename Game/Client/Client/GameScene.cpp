#include "pch.h"
#include "GameScene.h"
#include "GrenadeArcPass.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "TerrainObject.h"
#include "HelicopterObject.h"
#include "TerrainTestScene.h"
#include "Skybox.h"
#include "MapTestScene.h"
#include "LoginScene.h"
#include "MenuScene.h"
#include "LobbyScene.h"
#include "TextBox.h"
#include "Sprite.h"
#include "BloodEffect.h"
#include "MuzzleFlashEffect.h"
#include "ZombieAnimationController.h"
#include "ThirdPersonCamera.h"
#include "BulletImpactEffect.h"
#include "EventSequence.h"

void GameScene::BuildObjects()
{
	using namespace std::chrono;

	m_pUIBoard = std::make_unique<UIBoard>();
	m_pPlayer = std::make_shared<NetworkOwnerThirdPersonPlayer>();

	m_pSkybox = std::make_shared<Skybox>();

	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("Game");

	// 풀은 항상 dormant로 시작 — 오프라인은 스폰 포인트마다, 온라인은 서버 이벤트로 Acquire.
	// 엔딩 서바이벌 동안 서버가 최대 250까지 스폰하므로 풀 용량을 맞춰 둔다.
	m_ZombiePool.Initialize(250, true);

	auto begin = high_resolution_clock::now();
	LoadFromFiles("Game");
	auto end = high_resolution_clock::now();
	long long llLoadTime = duration_cast<milliseconds>(end - begin).count();

	// 좀비 스폰 포인트는 씬과 분리된 별도 JSON에서 로드 (언리얼 SaveSpawnPointsToJson 출력).
	// 오프라인은 UpdateOfflineSpawner()가 드립 방식으로 채운다 (online은 서버가 담당).
	LoadZombieSpawnPoints("GAME_SpawnPoints");
	// 헬기 추락 컷씬 비행 경로 (언리얼 HeliPath_N TargetPoint 익스포트). 없으면 직선 폴백.
	LoadHeliPath("GAME_HeliPath");
	// 구조 헬기(착륙) 비행 경로 (언리얼 ArrivePath_N TargetPoint 익스포트). 없으면 직선 폴백.
	LoadHeliArrivePath("GAME_HeliArrivePath");

	// 서버 트리거 게임 이벤트용 시퀀스 (런타임에 이벤트 AddEvent).
	// Scene::FixedUpdate()가 매 프레임 m_pEventSequence->Update() 호출.
	m_pEventSequence = std::make_shared<EventSequence>(this);
	m_pEventSequence->AddEvent(std::make_shared<FireEvent>());

	m_pWater = std::make_shared<WaterGridObject>();
	auto& pTransform = m_pWater->GetTransform();
	m_v3WaterPos = Vector3(0.f, -47_m, 0.f);
	pTransform->SetPosition(m_v3WaterPos);
	AddObject(m_pWater);

	m_ToneMappingVolume.LoadFromFiles("Start");
	m_PostProcessingVolume.LoadFromFiles("Start");
}

void GameScene::FinalizeBuild()
{
	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();
	m_pPlayer->Initialize();
	m_pPlayer->GetTransform()->SetPosition(10281.199179, -3536.692724, 18949.001705);
	//m_pPlayer->GetTransform()->SetPosition(29000, -3536.692724, 25000);
	if (auto pThirdPerson = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		const auto& data = GCTX->GetGameData();
		pThirdPerson->SetPlayerModel(GameContext::g_strCharacterNames[data.m_nCurModelIndex]);
		pThirdPerson->SetWeaponSlots((WEAPON_TYPE)data.m_nWeapon1Index, (WEAPON_TYPE)data.m_nWeapon2Index);
		if (bOnline)
			NETWORK->SendPlayerCharacter(static_cast<unsigned char>(data.m_nCurModelIndex)); // late-join 리모트에 모델 반영
	}

	m_pSkybox->Initialize();
	m_pSkybox->LoadSkyboxParameters("Start");

	BuildChatUI();

	// 전원 로딩 대기 안내 (화면 중앙). OnEnterScene에서 온라인일 때만 표시.
	m_pLoadWaitText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pLoadWaitText->SetText(L"다른 플레이어를 기다리는 중...");
	m_pLoadWaitText->SetLayer(0);
	m_pLoadWaitText->SetAnchor(Vector2{ 0.5f, 0.5f });   // 화면 중앙
	m_pLoadWaitText->SetPivot(Vector2{ 0.5f, 0.5f });
	m_pLoadWaitText->SetPosition(Vector2{ 0.f, 0.f });
	m_pLoadWaitText->SetTextHeight(48.f);
	m_pLoadWaitText->SetVisible(false);
	m_pUIBoard->InsertUI(m_pLoadWaitText);

	// 탈출 시퀀스 안내 HUD (총알/체력 UI와 동일하게 TextBox → UIBoard). 평소엔 숨김.
	m_pEscapeText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pEscapeText->SetText(L"");
	m_pEscapeText->SetLayer(0);
	m_pEscapeText->SetAnchor(Vector2{ 0.5f, 0.f });   // 화면 상단 중앙
	m_pEscapeText->SetPivot(Vector2{ 0.5f, 0.f });
	m_pEscapeText->SetPosition(Vector2{ 0.f, 60.f });
	m_pEscapeText->SetTextHeight(60.f);
	m_pEscapeText->SetVisible(false);
	m_pUIBoard->InsertUI(m_pEscapeText);

	// 관전/부활 HUD (화면 우상단). 죽어서 관전 중일 때만 표시.
	m_pSpectateText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pSpectateText->SetText(L"");
	m_pSpectateText->SetLayer(0);
	m_pSpectateText->SetAnchor(Vector2{ 1.f, 0.f });   // 화면 우상단
	m_pSpectateText->SetPivot(Vector2{ 1.f, 0.f });
	m_pSpectateText->SetPosition(Vector2{ -30.f, 30.f });
	m_pSpectateText->SetTextHeight(36.f);
	m_pSpectateText->SetVisible(false);
	m_pUIBoard->InsertUI(m_pSpectateText);

	// ── 전원 로딩 동기화 시작 ────────────────────────────────────────────────
	// 씬 셋업(플레이어/UI)이 모두 끝난 여기서 서버에 로딩 완료를 알리고,
	// S2C_GAME_BEGIN 수신까지 입력 차단(PostProcessInput) + 중앙 대기 안내 표시.
	// OnEnterScene은 FinalizeBuild보다 먼저 호출되므로 여기서 해야 UI가 존재한다.
	if (bOnline) {
		m_bWaitingAllLoaded = true;
		m_pLoadWaitText->SetVisible(true);

		NETWORK->ConsumeGameBegin(); // 이전 게임의 잔여 신호 제거 (재시작 대비)
		NETWORK->SendLoadComplete();
	}

	// 기존 HUD보다 나중에 삽입해 같은 최상위 레이어 안에서 Pause 오버레이가
	// HUD 위에 그려지도록 한다. HUD 자체는 온라인 상태 갱신을 위해 계속 유지한다.
	BuildPauseMenuUI();
}

void GameScene::OnEnterScene()
{
}

void GameScene::PostProcessInput()
{
	if (m_bPauseMenuOpen) {
		if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			pThirdPerson->ClearMovementInput();
		}
		m_World.PostProcessInput();
		return;
	}

	// 전원 로딩 대기 중: 플레이어 입력만 차단 — 카메라/물리/월드/UI 업데이트는
	// 정상 구동되어 화면·대기 안내가 제대로 나온다 (시네마틱 정지를 쓰면
	// 카메라 갱신까지 멈춰 초기 뷰(원점)에 방치됨).
	if (m_bWaitingAllLoaded) {
		if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			pThirdPerson->ClearMovementInput(); // 이동 입력 잔상 제거 (컷씬 차단과 동일)
		}
		return;
	}
	Scene::PostProcessInput();
}

void GameScene::OnLeaveScene()
{
	if (m_bPauseMenuOpen) {
		// 메뉴 씬으로 나갈 때는 게임용 Mouse Look 상태를 복원하지 않는다.
		m_bRestoreMouseLook = false;
		SetPauseMenuOpen(false);
	}
	ClearEndCreditsUI();
	GrenadeArcPass::SetArcVertices({});	// 예측 궤도 잔상 제거
}

void GameScene::ProcessInput()
{
	if (INPUT->GetButtonDown(VK_ESCAPE) && !m_bEndCreditsPlaying) {
		SetPauseMenuOpen(!m_bPauseMenuOpen);
	}
}

void GameScene::Update()
{
	if (m_bReturnToMenu) {
		m_bReturnToMenu = false;
		m_bRestoreMouseLook = false;
		SetPauseMenuOpen(false);
		SCENE->ChangeScene<MenuScene>();
		return;
	}

	//ImGui::Begin("Test");
	//{
	//	if (ImGui::Button("Change Scene")) {
	//		SCENE->ChangeScene<MapTestScene>();
	//		ImGui::End();
	//		return;
	//	}
	//
	//	ImGui::InputFloat3("Set Pos", reinterpret_cast<float*>(&v3PlayerPos));
	//	if (ImGui::Button("Move To Pos")) {
	//		m_pPlayer->GetTransform()->SetPosition(v3PlayerPos);
	//	}
	//
	//	if (ImGui::Button("Move To PlayerStart")) {
	//		m_pPlayer->GetTransform()->SetPosition(10281.199179, -3536.692724, 18949.001705);
	//	}
	//
	//	if (ImGui::BeginTabBar("Debug")) {
	//		if (ImGui::BeginTabItem("Player")) {
	//			if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
	//				ImGui::Text("Press `(~) to use mouse control");
	//				ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");
	//
	//				ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());
	//
	//				const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
	//				ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);
	//
	//				const auto& transform = pPlayer->GetTransform();
	//				Vector3 v3PlayerPos = transform->GetPosition();
	//				ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);
	//
	//				ImGui::Text("====== Collision Result ======");
	//				for (const auto& pair : m_pCollisionPairs) {
	//					ImGui::Text("Collision {%s : %s}", pair.pSelf->GetName().c_str(), pair.pOther->GetName().c_str());
	//				}
	//
	//			}
	//			else {
	//				ImGui::Text("No Animation");
	//			}
	//			ImGui::EndTabItem();
	//		}
	//		if (ImGui::BeginTabItem("Lights")) {
	//			ImGui::Text("Elapsed TIme : %f", TIME->GetTimeElapsed());
	//			ImGui::Text("Total TIme : %f", TIME->GetTotalTime());
	//
	//			float fAmbient = m_v4GlobalAmbient.x;
	//			ImGui::DragFloat("GlobalAmbient", (float*)&fAmbient, 0.001f, 0.f, 1.f);
	//			m_v4GlobalAmbient = XMVectorReplicate(fAmbient);
	//			ImGui::Text("NumLights : %d", m_pLights.size());
	//			for (uint32 i = 0; i < m_pLights.size(); ++i) {
	//				if (ImGui::TreeNode(std::format("Index : {}", i).c_str())) {
	//					m_pLights[i]->ShowControllImGui();
	//					ImGui::TreePop();
	//				}
	//			}
	//			ImGui::EndTabItem();
	//		}
	//		if (ImGui::BeginTabItem("Skybox")) {
	//			if (m_pSkybox) {
	//				m_pSkybox->ShowControllImGui();
	//			}
	//			ImGui::EndTabItem();
	//		}
	//
	//		if (ImGui::BeginTabItem("Objects")) {
	//			for (const auto& pObj : m_World.GetObjects<StaticObject>()) {
	//				if (ImGui::TreeNode(pObj->GetName().c_str())) {
	//					auto pTransform = pObj->GetTransform();
	//					const Vector3 v3Position = pTransform->GetPosition();
	//					ImGui::Text("Position : (%f, %f, %f)", v3Position.x, v3Position.y, v3Position.z);
	//
	//					pObj->ShowControlImGui();
	//
	//					ImGui::TreePop();
	//				}
	//			}
	//
	//			ImGui::EndTabItem();
	//		}
	//
	//		if (ImGui::BeginTabItem("Terrain")) {
	//			ImGui::DragFloat3("Terrain Position", (float*)&v3TerrainPos, 0.1f);
	//			ImGui::DragFloat3("Terrain Rotation", (float*)&v3TerrainRotation, 0.1f);
	//
	//			m_pTerrain->GetTransform()->SetPosition(v3TerrainPos);
	//			m_pTerrain->GetTransform()->SetRotation(v3TerrainRotation);
	//
	//			ImGui::EndTabItem();
	//		}
	//
	//		ImGui::EndTabBar();
	//	}
	//
	//}
	//ImGui::End();

	// ── 좀비 네트워크 디버그 ────────────────────────────────────────────────
	//ImGui::Begin("Zombie Network Debug");
	//{
	//	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();
	//	ImGui::Text("Online: %s", bOnline ? "YES" : "NO");
	//	ImGui::Text("Connected: %s", NETWORK->IsConnected() ? "YES" : "NO");
	//	ImGui::Text("Offline: %s", NETWORK->IsOffline() ? "YES" : "NO");
	//	ImGui::Text("Pool Active: %d / Free: %d", m_ZombiePool.GetActiveCount(), m_ZombiePool.GetFreeCount());
	//	ImGui::Text("ServerZombies map: %d", (int)m_ServerZombies.size());
	//	for (auto& [nId, pZ] : m_ServerZombies)
	//	{
	//		if (!pZ) continue;
	//		Vector3 v3Pos = pZ->GetTransform()->GetPosition();
	//		ImGui::Text("  [%d] pos(%.0f, %.0f, %.0f) active=%d hp=%.0f",
	//			nId, v3Pos.x, v3Pos.y, v3Pos.z, pZ->IsPoolActive() ? 1 : 0, pZ->GetHP());
	//	}
	//}
	//ImGui::End();

	// ====== Test ======
	ImGui::Begin("Test");
	{
		if (ImGui::Button("Play 2D")) {
			SOUND->Play("Test");
		}
		ImGui::SameLine();
		if (ImGui::Button("Play 3D @player")) {
			Vector3 v3Pos = m_pPlayer ? m_pPlayer->GetTransform()->GetPosition() : Vector3::Zero;
			SOUND->PlayAt("Test3D", v3Pos);
		}

		// 3D direction test: fire 3000cm (30m) from player in world axes.
		Vector3 v3Base = m_pPlayer ? m_pPlayer->GetTransform()->GetPosition() : Vector3::Zero;
		const float fDist = 3_m;
		if (ImGui::Button("Left  (-X)")) {
			SOUND->PlayAt("Test3D", v3Base + Vector3(-fDist, 0.0f, 0.0f));
		}
		ImGui::SameLine();
		if (ImGui::Button("Right (+X)")) {
			SOUND->PlayAt("Test3D", v3Base + Vector3(fDist, 0.0f, 0.0f));
		}
		if (ImGui::Button("Front (+Z)")) {
			SOUND->PlayAt("Test3D", v3Base + Vector3(0.0f, 0.0f, fDist));
		}
		ImGui::SameLine();
		if (ImGui::Button("Back  (-Z)")) {
			SOUND->PlayAt("Test3D", v3Base + Vector3(0.0f, 0.0f, -fDist));
		}

		float fSfx = SOUND->GetCategoryVolume(SoundCategory::SFX);
		if (ImGui::SliderFloat("SFX Vol", &fSfx, 0.0f, 1.0f)) {
			SOUND->SetCategoryVolume(SoundCategory::SFX, fSfx);
		}
		float fMaster = SOUND->GetMasterVolume();
		if (ImGui::SliderFloat("Master Vol", &fMaster, 0.0f, 1.0f)) {
			SOUND->SetMasterVolume(fMaster);
		}
		if (ImGui::Button("End Credits Test")) {
			BeginEndCredits();
		}

		if (ImGui::Button("Env Preset 1")) {
			// 환경 프리셋 전환: 한 이벤트로 포스트FX/안개/시간/앰비언트 전체를 페이드.
			m_pEventSequence->AddEvent(std::make_shared<EnvironmentTransitionEvent>(GetEnvironmentPreset(1), 1.5f));
		}

		if (ImGui::Button("Env Preset 2")) {
			// 환경 프리셋 전환: 한 이벤트로 포스트FX/안개/시간/앰비언트 전체를 페이드.
			m_pEventSequence->AddEvent(std::make_shared<EnvironmentTransitionEvent>(GetEnvironmentPreset(2), 1.5f));
		}

		if (ImGui::Button("Env Preset 3")) {
			// 환경 프리셋 전환: 한 이벤트로 포스트FX/안개/시간/앰비언트 전체를 페이드.
			m_pEventSequence->AddEvent(std::make_shared<EnvironmentTransitionEvent>(GetEnvironmentPreset(3), 1.5f));
		}

	}
	ImGui::End();

	if (m_bEndCreditsPlaying || m_bGameCleared) {
		if (m_bGameCleared && !m_bEndCreditsPlaying && !m_bEndCreditsFinished) {
			BeginEndCredits();
		}
		if (m_bEndCreditsPlaying) {
			UpdateEndCredits();
		}
		return;
	}

	// 전원 로딩 대기 — 서버의 동시 시작 신호(또는 연결 끊김) 시 입력 차단 해제
	if (m_bWaitingAllLoaded) {
		if (NETWORK->ConsumeGameBegin() || !NETWORK->IsConnected()) {
			m_bWaitingAllLoaded = false;
			if (m_pLoadWaitText) {
				m_pLoadWaitText->SetVisible(false);
			}
			// 장벽 해제 직후 유예 갱신 — 느리게 로딩한 플레이어의 초기 무기 장착
			// 브로드캐스트가 이제야 소비될 수 있어, 드로우 모션 억제 창을 다시 연다
			const float fNow = NetworkManager::GetNetTimeSec();
			for (auto& [id, fSpawnTime] : m_RemoteSpawnTimes)
				fSpawnTime = fNow;
		}
	}

	UpdateOfflineSpawner();
	SyncSceneWithServer();
	ProcessNetworkZombies();
	ProcessPlayerMelee();
	ProcessPlayerBandage();
	ProcessPlayerGrenade();
	UpdateGrenadeArcPreview();
	ProcessShootResults();
	ProcessMeleeResults();
	ProcessBandageResults();
	ProcessGrenadeResults();
	UpdateGrenades();
	ProcessServerGameEvents();
	UpdateDeathAndSpectate();
	UpdateEscapeSequence();
	RemoveDeadZombies();
	UpdateChat();
}

void GameScene::BuildPauseMenuUI()
{
	if (!m_pUIBoard) return;

	auto fnAddPauseComponent = [this](const std::shared_ptr<IUIComponent>& pComponent) {
		pComponent->SetVisible(false);
		m_pUIBoard->InsertUI(pComponent);
		m_pPauseMenuComponents.push_back(pComponent);
	};

	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);

	std::shared_ptr<ImageBox> pBackground = std::make_shared<ImageBox>("Color");
	pBackground->SetLayer(0);
	pBackground->SetAnchor(Vector2{ 0.5f, 0.5f });
	pBackground->SetPivot(Vector2{ 0.5f, 0.5f });
	pBackground->SetSize(Vector2{ fScreenWidth, fScreenHeight });
	pBackground->SetColor(Vector4{ 0.0f, 0.0f, 0.0f, 0.72f });
	fnAddPauseComponent(pBackground);

	std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Noto Sans KR");
	pTitle->SetText(L"PAUSED");
	pTitle->SetLayer(0);
	pTitle->SetAnchor(Vector2{ 0.5f, 0.5f });
	pTitle->SetPivot(Vector2{ 0.5f, 0.5f });
	pTitle->SetPosition(Vector2{ 0.f, -190.f });
	pTitle->SetTextHeight(72.f);
	fnAddPauseComponent(pTitle);

	auto fnMakeButton = [&fnAddPauseComponent](const std::wstring& wstrText, const Vector2& v2Position, auto fnCallback) {
		std::shared_ptr<TextButton> pButton = std::make_shared<TextButton>(L"Noto Sans KR");
		pButton->SetText(wstrText);
		pButton->SetLayer(0);
		pButton->SetAnchor(Vector2{ 0.5f, 0.5f });
		pButton->SetPivot(Vector2{ 0.5f, 0.5f });
		pButton->SetPosition(v2Position);
		pButton->SetTextHeight(46.f);
		pButton->SetButtonCallback(std::move(fnCallback));
		fnAddPauseComponent(pButton);
	};

	fnMakeButton(L"RESUME", Vector2{ 0.f, -85.f }, [this](IUIComponent*) {
		SetPauseMenuOpen(false);
	});
	fnMakeButton(L"RETURN TO MENU", Vector2{ 0.f, -20.f }, [this](IUIComponent*) {
		m_bReturnToMenu = true;
	});

	std::shared_ptr<TextBox> pVolumeLabel = std::make_shared<TextBox>(L"Noto Sans KR");
	pVolumeLabel->SetText(L"VOLUME");
	pVolumeLabel->SetLayer(0);
	pVolumeLabel->SetAnchor(Vector2{ 0.5f, 0.5f });
	pVolumeLabel->SetPivot(Vector2{ 0.5f, 0.5f });
	pVolumeLabel->SetPosition(Vector2{ -110.f, 80.f });
	pVolumeLabel->SetTextHeight(38.f);
	fnAddPauseComponent(pVolumeLabel);

	m_pPauseVolumeText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pPauseVolumeText->SetLayer(0);
	m_pPauseVolumeText->SetAnchor(Vector2{ 0.5f, 0.5f });
	m_pPauseVolumeText->SetPivot(Vector2{ 0.5f, 0.5f });
	m_pPauseVolumeText->SetPosition(Vector2{ 65.f, 80.f });
	m_pPauseVolumeText->SetTextHeight(38.f);
	fnAddPauseComponent(m_pPauseVolumeText);
	RefreshPauseVolumeText();

	fnMakeButton(L"<<", Vector2{ 0.f, 80.f }, [this](IUIComponent*) {
		SOUND->SetMasterVolume(SOUND->GetMasterVolume() - SOUND_WEIGHT);
		RefreshPauseVolumeText();
	});
	fnMakeButton(L">>", Vector2{ 130.f, 80.f }, [this](IUIComponent*) {
		SOUND->SetMasterVolume(SOUND->GetMasterVolume() + SOUND_WEIGHT);
		RefreshPauseVolumeText();
	});
	fnMakeButton(L"QUIT GAME", Vector2{ 0.f, 180.f }, [](IUIComponent*) {
		::PostQuitMessage(0);
	});
}

void GameScene::SetPauseMenuOpen(bool bOpen)
{
	if (m_bPauseMenuOpen == bOpen) return;

	m_bPauseMenuOpen = bOpen;
	for (const auto& pComponent : m_pPauseMenuComponents) {
		if (pComponent) {
			pComponent->SetVisible(m_bPauseMenuOpen);
		}
	}
	if (m_pChatInput) {
		m_pChatInput->SetVisible(!m_bPauseMenuOpen);
	}

	if (m_pUIBoard) {
		m_pUIBoard->ClearFocus();
	}
	INPUT->SetTextInputMode(false);

	auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (m_bPauseMenuOpen) {
		if (pThirdPerson) {
			pThirdPerson->ClearMovementInput();
			pThirdPerson->LeaveAim();
			m_bRestoreMouseLook = pThirdPerson->IsMouseOn();
			pThirdPerson->SetMouseLookEnabled(false);
		}
		else {
			m_bRestoreMouseLook = false;
			::ClipCursor(nullptr);
			INPUT->ShowCursor();
		}
		RefreshPauseVolumeText();
	}
	else if (pThirdPerson && m_bRestoreMouseLook) {
		pThirdPerson->SetMouseLookEnabled(true);
		m_bRestoreMouseLook = false;
	}
}

void GameScene::RefreshPauseVolumeText()
{
	if (!m_pPauseVolumeText) return;
	const int nPercent = static_cast<int>(std::round(SOUND->GetMasterVolume() * 100.f));
	m_pPauseVolumeText->SetText(std::to_wstring(nPercent));
}

void GameScene::BeginEndCredits()
{
	m_bEndCreditsPlaying = true;
	m_bEndCreditsFinished = false;
	m_fEndCreditsElapsed = 0.0f;

	if (m_pUIBoard) {
		m_pUIBoard->ClearFocus();
	}
	INPUT->SetTextInputMode(false);

	BuildEndCreditsUI();
}

void GameScene::BuildEndCreditsUI()
{
	ClearEndCreditsUI();

	if (!m_pUIBoard) {
		return;
	}

	const std::wstring wstrTitle = L"THE END";
	const std::vector<std::wstring> wstrcreditLines = {
		L"Thanks for playing",
		L"",
		L"Client framework",
		L"- 이승욱",
		L"",
		L"Client / Server contents",
		L"- 민정원",
		L"",
		L"Server",
		L"- 최명준",
		L"",
		L"Asset optimize / Map design",
		L"- 이동연",
		L"",
	};

	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);
	const float fBaseY = fScreenHeight * 0.5f + 120.0f;

	m_pEndBackgroundImage = std::make_shared<ImageBox>("Color");
	m_pEndBackgroundImage->SetLayer(0);
	m_pEndBackgroundImage->SetAnchor(Vector2{ 0.5f, 0.5f });
	m_pEndBackgroundImage->SetPivot(Vector2{ 0.5f, 0.5f });
	m_pEndBackgroundImage->SetPosition(Vector2{ 0.0f, 0.0f });
	m_pEndBackgroundImage->SetSize(Vector2{ fScreenWidth, fScreenHeight });
	m_pEndBackgroundImage->SetColor(Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
	m_pUIBoard->InsertUI(m_pEndBackgroundImage);

	m_pEndTitleText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pEndTitleText->SetText(wstrTitle);
	m_pEndTitleText->SetLayer(0);
	m_pEndTitleText->SetAnchor(Vector2{ 0.5f, 0.5f });
	m_pEndTitleText->SetPivot(Vector2{ 0.5f, 0.5f });
	m_pEndTitleText->SetPosition(Vector2{ 0.0f, fBaseY });
	m_pEndTitleText->SetTextHeight(72.0f);
	m_pEndTitleText->SetColor(Vector4{ 1.0f, 1.0f, 1.0f, 1.0f });
	m_pUIBoard->InsertUI(m_pEndTitleText);

	for (size_t i = 0; i < wstrcreditLines.size(); ++i) {
		auto pText = std::make_shared<TextBox>(L"Noto Sans KR");
		pText->SetText(wstrcreditLines[i]);
		pText->SetLayer(0);
		pText->SetAnchor(Vector2{ 0.5f, 0.5f });
		pText->SetPivot(Vector2{ 0.5f, 0.5f });
		pText->SetPosition(Vector2{ 0.0f, fBaseY + END_CREDITS_FIRST_LINE_OFFSET_Y + static_cast<float>(i) * END_CREDITS_LINE_HEIGHT });
		pText->SetTextHeight(wstrcreditLines[i].empty() ? 12.0f : 26.0f);
		pText->SetColor(Vector4{ 0.9f, 0.9f, 0.9f, 1.0f });
		m_pUIBoard->InsertUI(pText);
		m_pEndCreditTexts.push_back(pText);
	}
}

void GameScene::UpdateEndCredits()
{
	m_fEndCreditsElapsed += DT;

	float fFadeT = m_fEndCreditsElapsed / END_CREDITS_FADE_IN_DURATION;
	if (fFadeT > 1.0f) {
		fFadeT = 1.0f;
	}

	float fScrollT = m_fEndCreditsElapsed / END_CREDITS_DURATION;
	if (fScrollT > 1.0f) {
		fScrollT = 1.0f;
	}

	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);
	const float fCreditHeight = END_CREDITS_FIRST_LINE_OFFSET_Y
		+ static_cast<float>(m_pEndCreditTexts.size()) * END_CREDITS_LINE_HEIGHT
		+ 120.0f;
	const float fStartY = fScreenHeight * 0.5f + 120.0f;
	const float fEndY = -fScreenHeight * 0.5f - fCreditHeight;
	const float fBaseY = fStartY + (fEndY - fStartY) * fScrollT;

	if (m_pEndBackgroundImage) {
		m_pEndBackgroundImage->SetSize(Vector2{ fScreenWidth, fScreenHeight });
		m_pEndBackgroundImage->SetColor(Vector4{ 0.0f, 0.0f, 0.0f, fFadeT });
	}

	if (m_pEndTitleText) {
		m_pEndTitleText->SetPosition(Vector2{ 0.0f, fBaseY });
	}

	for (size_t i = 0; i < m_pEndCreditTexts.size(); ++i) {
		if (m_pEndCreditTexts[i]) {
			m_pEndCreditTexts[i]->SetPosition(Vector2{
				0.0f,
				fBaseY + END_CREDITS_FIRST_LINE_OFFSET_Y + static_cast<float>(i) * END_CREDITS_LINE_HEIGHT
			});
		}
	}

	if (m_fEndCreditsElapsed >= END_CREDITS_DURATION) {
		ClearEndCreditsUI();
		m_bEndCreditsPlaying = false;
		m_bEndCreditsFinished = true;
		SCENE->PopScene();
		SCENE->PushScene<LogInScene>();
		SCENE->PushScene<MenuScene>();
		SCENE->PushScene<LobbyScene>();
	}
}

void GameScene::ClearEndCreditsUI()
{
	if (m_pUIBoard) {
		if (m_pEndBackgroundImage) {
			m_pUIBoard->RemoveUI(m_pEndBackgroundImage);
		}

		if (m_pEndTitleText) {
			m_pUIBoard->RemoveUI(m_pEndTitleText);
		}

		for (const auto& pText : m_pEndCreditTexts) {
			if (pText) {
				m_pUIBoard->RemoveUI(pText);
			}
		}
	}

	m_pEndBackgroundImage.reset();
	m_pEndTitleText.reset();
	m_pEndCreditTexts.clear();
}

void GameScene::ProcessServerGameEvents()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;
	if (!m_pEventSequence) return;

	for (const auto& ev : NETWORK->ConsumeGameEvents())
	{
		switch (ev.eventId)
		{
		case GE_EXPLOSION:
			m_pEventSequence->AddEvent(std::make_shared<ExplosionEvent>(ev.pos));
			break;
		case GE_ENVIRONMENT:
			// 환경 프리셋 전환: 한 이벤트로 포스트FX/안개/시간/앰비언트 전체를 페이드.
			m_pEventSequence->AddEvent(std::make_shared<EnvironmentTransitionEvent>(
				GetEnvironmentPreset(ev.presetId), ev.fDuration > 0.f ? ev.fDuration : 1.5f));
			break;
		case GE_HELICOPTER_CRASH:
			// 헬기 추락 컷씬: 시네마틱 카메라가 추락 헬기 추적 (fDuration=0 → 기본 5초)
			m_pEventSequence->AddEvent(std::make_shared<HelicopterCrashEvent>(ev.fDuration));
			break;
		case GE_HELICOPTER_ARRIVE:
			// 구조 헬기 강하·착륙 컷씬(폭발 X). 착륙 후 탈출 시퀀스로 이어짐(2단계에서 처리).
			m_pArriveEvent = std::make_shared<HelicopterCrashEvent>(ev.fDuration, /*bLandMode=*/true);
			m_pEventSequence->AddEvent(m_pArriveEvent);
			break;
		default:
			break;
		}
	}
}

// ── 사망/관전/부활 ───────────────────────────────────────────────────────────
// 온라인: 서버가 HP 차감/사망 판정/부활 타이머를 주관 (S2C_PLAYER_DEATH / S2C_PLAYER_RESPAWN).
// 오프라인: 로컬 HP로 사망 판정 + 로컬 타이머로 제자리 부활.
// 관전: 죽으면 카메라 owner를 살아있는 리모트로 교체, 좌클릭으로 다음 대상 순환.
void GameScene::UpdateDeathAndSpectate()
{
	const bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();

	if (bOnline)
	{
		for (const auto& ev : NETWORK->ConsumePlayerDeaths())
		{
			if (ev.playerId == NETWORK->GetPlayerID()) {
				m_fRespawnRemain = ev.fRespawnSeconds;
				if (auto pTP = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer))
					pTP->ApplyDead();	// 사망 모션 (관전 중 시체에 보임)
				if (!m_bSpectating) EnterSpectateMode();
			}
			else {
				m_DeadPlayers.insert(ev.playerId);
				auto it = m_RemotePlayers.find(ev.playerId);
				if (it != m_RemotePlayers.end() && it->second)
					it->second->ApplyDead();			// 리모트 사망 모션
				// 보고 있던 대상이 죽으면 다음 대상으로
				if (m_bSpectating && m_nSpectateTargetId == ev.playerId)
					CycleSpectateTarget();
			}
		}

		for (const auto& ev : NETWORK->ConsumePlayerRespawns())
		{
			if (ev.playerId == NETWORK->GetPlayerID()) {
				if (m_bSpectating) LeaveSpectateMode(&ev.pos);
			}
			else {
				m_DeadPlayers.erase(ev.playerId);
				auto it = m_RemotePlayers.find(ev.playerId);
				if (it != m_RemotePlayers.end() && it->second)
					it->second->ApplyRespawn();			// 리모트 사망 몽타주 해제
			}
		}

		if (m_bSpectating)
			m_fRespawnRemain = std::max(0.f, m_fRespawnRemain - DT);
	}
	else
	{
		// 오프라인: 로컬 사망 감지 + 로컬 부활 타이머 (리모트가 없으므로 내 시체 시점 유지)
		if (!m_bSpectating && m_pPlayer && m_pPlayer->IsDead()) {
			m_fRespawnRemain = OFFLINE_RESPAWN_SECONDS;
			std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)->ApplyDead();	// 사망 모션
			EnterSpectateMode();
		}
		if (m_bSpectating) {
			m_fRespawnRemain -= DT;
			if (m_fRespawnRemain <= 0.f) {
				LeaveSpectateMode(nullptr);
				return;
			}
		}
	}

	if (!m_bSpectating) {
		if (m_pSpectateText) m_pSpectateText->SetVisible(false);
		return;
	}

	// 관전 대상이 방을 나갔으면 다음 대상으로
	if (m_nSpectateTargetId >= 0 && !m_RemotePlayers.contains(m_nSpectateTargetId))
		CycleSpectateTarget();

	// 좌클릭 → 다음 관전 대상 순환
	if (!m_bPauseMenuOpen && !INPUT->IsTextInputMode() && INPUT->GetButtonDown(VK_LBUTTON))
		CycleSpectateTarget();

	// ── 우상단 HUD: 관전 대상 이름 + 부활 카운트다운 ─────────────────────────
	if (!m_pSpectateText) return;
	m_pSpectateText->SetVisible(true);
	m_pSpectateText->SetColor(Vector3{ 1.f, 0.85f, 0.3f });

	const int nRemain = static_cast<int>(std::ceil(std::max(0.f, m_fRespawnRemain)));
	if (m_nSpectateTargetId >= 0)
	{
		// 대상 이름 조회 (방 멤버 스냅샷). 못 찾으면 playerId 표시.
		std::wstring wstrName = std::to_wstring(m_nSpectateTargetId);
		for (const auto& ev : NETWORK->GetRoomPlayersSnapshot()) {
			if (ev.playerId == m_nSpectateTargetId) {
				wstrName = StringToWString(ev.username);
				break;
			}
		}
		m_pSpectateText->SetText(std::format(L"관전 중: {}  |  부활까지 {}초  [좌클릭: 다음]", wstrName, nRemain));
	}
	else
	{
		m_pSpectateText->SetText(std::format(L"부활까지 {}초", nRemain));
	}
}

void GameScene::EnterSpectateMode()
{
	m_bSpectating = true;
	m_nSpectateTargetId = -1;
	CycleSpectateTarget(); // 살아있는 리모트가 있으면 바로 관전, 없으면 내 시체 시점
}

void GameScene::LeaveSpectateMode(const Vector3* respawnPos)
{
	m_bSpectating = false;
	m_nSpectateTargetId = -1;
	m_fRespawnRemain = 0.f;

	if (m_pPlayer) {
		m_pPlayer->RestoreFullHP();
		std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)->ApplyRespawn();	// 사망 몽타주(FREEZE) 해제
		if (respawnPos)
			m_pPlayer->GetTransform()->SetPosition(*respawnPos);
		if (m_pPlayer->GetCamera())
			m_pPlayer->GetCamera()->SetOwner(m_pPlayer); // 카메라 원복
	}
	if (m_pSpectateText) m_pSpectateText->SetVisible(false);
}

bool GameScene::CycleSpectateTarget()
{
	if (!m_pPlayer || !m_pPlayer->GetCamera()) return false;

	// 살아있는 리모트 후보 (id 오름차순 순환)
	std::vector<int> candidates;
	candidates.reserve(m_RemotePlayers.size());
	for (const auto& [id, pRemote] : m_RemotePlayers)
		if (pRemote && !m_DeadPlayers.contains(id))
			candidates.push_back(id);

	if (candidates.empty()) {
		m_nSpectateTargetId = -1;
		m_pPlayer->GetCamera()->SetOwner(m_pPlayer); // 후보 없음 → 내 시체 시점
		return false;
	}

	std::sort(candidates.begin(), candidates.end());
	int nNext = candidates.front();
	for (size_t i = 0; i < candidates.size(); ++i) {
		if (candidates[i] == m_nSpectateTargetId) {
			nNext = candidates[(i + 1) % candidates.size()];
			break;
		}
	}

	m_nSpectateTargetId = nNext;
	m_pPlayer->GetCamera()->SetOwner(m_RemotePlayers[nNext]);
	return true;
}

// 마지막 체크포인트 후 탈출 시퀀스 (서버 권위).
//  - 시간 카운트다운/상태/종료 판정은 모두 서버가 함 (S2C_ESCAPE_STATE / S2C_GAME_END).
//  - 클라는 받은 상태로 UI를 그리고, 탈출 가능 + 헬기 반경 안에서 F를 누르면 서버에 1회 전송.
//  - 누구든 1명이 보내면 서버가 전원에게 S2C_GAME_END → 모두 클리어.
void GameScene::UpdateEscapeSequence()
{
	if (!m_pEscapeText) return;

	// 출발 컷씬 종료 후 → 클리어 메시지 고정.
	if (m_bGameCleared) {
		m_pEscapeText->SetVisible(true);
		m_pEscapeText->SetColor(Vector3{ 0.4f, 1.f, 0.4f });
		m_pEscapeText->SetText(L"탈출 성공!");
		return;
	}

	// 출발 컷씬 진행 중 → HUD 숨기고, 컷씬이 끝나면 클리어.
	if (m_bDeparting) {
		m_pEscapeText->SetVisible(false);
		if (!m_pDepartEvent || m_pDepartEvent->IsFinished())
			m_bGameCleared = true;
		return;
	}

	// 서버 게임 종료 신호 수신 → 출발 컷씬 시작 (플레이어 숨김 + 헬기 역방향 상승 + 하늘 고정 카메라).
	if (NETWORK->ConsumeGameEnd()) {
		std::shared_ptr<HelicopterObject> pHeli = m_pArriveEvent ? m_pArriveEvent->GetHeli() : nullptr;
		std::vector<Vector3> reversed = GetHeliArrivePath();   // 착륙 경로(상공→착륙점)
		std::reverse(reversed.begin(), reversed.end());        // 역방향(착륙점→상공)

		if (pHeli && reversed.size() >= 2 && m_pEventSequence) {
			m_pDepartEvent = std::make_shared<HelicopterDepartEvent>(pHeli, std::move(reversed), 8.0f);
			m_pEventSequence->AddEvent(m_pDepartEvent);
			m_bDeparting = true;
		}
		else {
			m_bGameCleared = true; // 헬기/경로 없으면 컷씬 없이 바로 클리어
		}
		m_pEscapeText->SetVisible(false);
		return;
	}

	// 착륙 지점 1회 기록 (탈출 반경 판정용 — 클라 로컬).
	if (!m_bExtractionRecorded && m_pArriveEvent && m_pArriveEvent->IsLanded()) {
		m_v3ExtractionPos     = m_pArriveEvent->GetExtractionPos();
		m_bExtractionRecorded = true;
	}

	// 서버가 보낸 탈출 상태가 아직 없으면(시퀀스 전) HUD 숨김.
	unsigned char ucPhase = 0;
	float fRemain = 0.f;
	if (!NETWORK->GetEscapeState(ucPhase, fRemain)) {
		m_pEscapeText->SetVisible(false);
		return;
	}

	m_pEscapeText->SetVisible(true);

	if (ucPhase == 0) {
		// 서바이벌: 서버가 준 남은 시간 표시.
		const int nRemain = (int)std::ceil(fRemain);
		m_pEscapeText->SetColor(Vector3{ 1.f, 1.f, 1.f });
		m_pEscapeText->SetText(std::format(L"탈출까지 버텨라  {}:{:02d}", nRemain / 60, nRemain % 60));
	}
	else {
		// 탈출 가능: 헬기 반경 안에서 F → 서버에 1회 전송.
		bool bInRange = false;
		if (m_pPlayer && m_bExtractionRecorded) {
			const Vector3 v3PlayerPos = m_pPlayer->GetTransform()->GetPosition();
			bInRange = (v3PlayerPos - m_v3ExtractionPos).Length() <= ESCAPE_RADIUS;
		}

		if (bInRange) {
			m_pEscapeText->SetColor(Vector3{ 0.4f, 1.f, 0.4f });
			m_pEscapeText->SetText(L"[F] 탈출!");
			// 사망(관전) 중에는 탈출 불가 — 시체가 반경 안에 있어도 F 무시
			if (!m_bPauseMenuOpen && !m_bEscapeKeySent && !m_pPlayer->IsDead() && INPUT->GetButtonDown('F')) {
				NETWORK->SendPlayerEscape();
				m_bEscapeKeySent = true;
			}
		}
		else {
			m_pEscapeText->SetColor(Vector3{ 1.f, 0.9f, 0.3f });
			m_pEscapeText->SetText(L"헬기로 가서 F키로 탈출!");
		}
	}
}

void GameScene::BuildChatUI()
{
	const float fLineH  = 24.f;
	const float fInputH = 28.f;
	const float fLeft   = 20.f;

	// 입력창: 화면 좌하단 앵커, 피벗도 자기 좌하단
	m_pChatInput = std::make_shared<InputTextBox>(L"Noto Sans KR");
	m_pChatInput->SetPlaceholder(L"Press Enter to chat");
	m_pChatInput->SetLayer(0);
	m_pChatInput->SetAnchor(Vector2{ 0.f, 1.f });
	m_pChatInput->SetPivot(Vector2{ 0.f, 1.f });
	m_pChatInput->SetPosition(Vector2{ fLeft, -20.f });
	m_pChatInput->SetTextHeight(fInputH);
	m_pUIBoard->InsertUI(m_pChatInput);

	// 히스토리 줄: 입력창 위로 쌓임. 인덱스 0 = 맨 아래(최신), 클수록 과거
	for (int i = 0; i < CHAT_VISIBLE_LINES; ++i) {
		auto pLine = std::make_shared<TextBox>(L"Noto Sans KR");
		pLine->SetText(L"");
		pLine->SetLayer(0);
		pLine->SetAnchor(Vector2{ 0.f, 1.f });
		pLine->SetPivot(Vector2{ 0.f, 1.f });
		pLine->SetPosition(Vector2{ fLeft, -52.f - fLineH * i });
		pLine->SetTextHeight(fLineH);
		m_pUIBoard->InsertUI(pLine);
		m_pChatLines[i] = pLine;
	}
}

void GameScene::UpdateChat()
{
	if (!m_pChatInput) return;

	// 1) 네트워크 스레드가 큐에 넣은 수신 메시지를 히스토리로 흡수
	for (const ChatMessageEvent& ev : NETWORK->ConsumeChatMessages()) {
		std::wstring wstrLine =
			L"[" + StringToWString(ev.username) + L"] " + StringToWString(ev.message);
		m_ChatHistory.push_back(std::move(wstrLine));
		if (m_ChatHistory.size() > CHAT_MAX_HISTORY)
			m_ChatHistory.erase(m_ChatHistory.begin());
	}

	// 2) 표시 줄 갱신: 최신 메시지가 맨 아래 줄(인덱스 0)
	const int nHistory = static_cast<int>(m_ChatHistory.size());
	for (int i = 0; i < CHAT_VISIBLE_LINES; ++i) {
		int nIdx = nHistory - 1 - i;
		m_pChatLines[i]->SetText(nIdx >= 0 ? m_ChatHistory[nIdx] : std::wstring{});
	}

	// 3) Enter로 입력창 토글: 포커스 -> 타이핑 -> Enter로 전송 후 포커스 해제.
	//    키 입력과 그것이 변환된 WM_CHAR 사이의 이중 발동을 피하려고
	//    WM_CHAR enter 콜백이 아닌 폴링으로만 처리한다.
	if (!m_bPauseMenuOpen && INPUT->GetButtonDown(VK_RETURN)) {
		if (m_pChatInput->IsFocused()) {
			const std::wstring& wstrText = m_pChatInput->GetCommittedText();
			if (!wstrText.empty())
				NETWORK->SendChat(WStringToString(wstrText));
			m_pChatInput->ClearText();
			m_pUIBoard->ClearFocus();
		}
		else {
			m_pUIBoard->SetFocus(m_pChatInput);
		}
	}

	// 4) 채팅창이 포커스를 가진 동안 게임플레이 입력 차단
	INPUT->SetTextInputMode(m_pChatInput->IsFocused());
}

void GameScene::ProcessPlayerShoot()
{
	auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer || !pPlayer->ConsumeFire())
		return;

	auto pCamera = std::static_pointer_cast<ThirdPersonCamera>(pPlayer->GetCamera());
	Vector3 v3RayOrigin = pCamera->GetPosition();
	Vector3 v3RayDir = pCamera->GetLook();

	float fMinDist = std::numeric_limits<float>::max();
	std::shared_ptr<Zombie> pHitZombie;

	for (const auto& pZombie : m_World.GetObjects<Zombie>()) {
		auto pCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pCollider) continue;

		BoundingBox aabb;
		pCollider->GetCapsuleWorld().CreateAABBFromCapsule(aabb);

		float fDist = 0.f;
		if (aabb.Intersects(XMLoadFloat3(&v3RayOrigin), XMLoadFloat3(&v3RayDir), fDist)) {
			if (fDist < fMinDist) {
				fMinDist = fDist;
				pHitZombie = pZombie;
			}
		}
	}

	if (pHitZombie)
		pHitZombie->TakeDamage(25.f);
}

void GameScene::ProcessPlayerMelee()
{
	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer || !pPlayer->ConsumeMelee())
		return;

	auto pCamera = std::dynamic_pointer_cast<ThirdPersonCamera>(pPlayer->GetCamera());
	if (!pCamera)
		return;

	// origin은 발밑(Transform pos)이 아니라 캡슐 중심 높이를 써야 함.
	// 발밑 + 수평 레이는 좀비 히트 캡슐(몸통 높이) 아래로 빗나감.
	Vector3 v3Origin = pPlayer->GetTransform()->GetPosition();
	if (auto pSelfCollider = pPlayer->GetComponent<PlayerCollider>())
		v3Origin = pSelfCollider->GetCapsuleWorld().v3Center;

	Vector3 v3Dir = pCamera->GetForwardXZ();		// 전방 XZ (y=0)
	if (v3Dir.LengthSquared() > 1e-6f)
		v3Dir.Normalize();

	constexpr float fRange  = 200.f;
	constexpr float fDamage = 50.f;

	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();
	if (bOnline)
	{
		// 온라인: 서버가 권위적으로 판정 → S2C_MELEE_HIT 로 데미지 반영
		NETWORK->SendPlayerMelee(v3Origin, v3Dir);
		return;
	}

	// 오프라인: 단일 레이 — 전방 사거리 내 가장 가까운 좀비 1마리
	float fMinDist = fRange;
	std::shared_ptr<Zombie> pHitZombie;
	for (const auto& pZombie : m_World.GetObjects<Zombie>())
	{
		if (!pZombie || !pZombie->IsPoolActive())
			continue;

		auto pCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pCollider)
			continue;

		float fDist = 0.f;
		if (pCollider->GetCapsuleWorld().Intersects(v3Origin, v3Dir, fDist))
		{
			if (fDist >= 0.f && fDist < fMinDist)
			{
				fMinDist = fDist;
				pHitZombie = pZombie;
			}
		}
	}

	if (pHitZombie)
	{
		pHitZombie->TakeDamage(fDamage);

		auto pCollider = pHitZombie->GetComponent<PlayerCollider>();

		// 피 이펙트 (좀비 캡슐 중심)
		ParticleEffectSpawnDesc desc{};
		desc.v3Position = pCollider->GetCapsuleWorld().v3Center;
		desc.v3Direction = -v3Dir;
		desc.v3Normal = Vector3::Up;
		desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
		PARTICLE->Spawn<BloodEffect>(desc);
	}
}

void GameScene::ProcessMeleeResults()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);

	// ── 리모트 플레이어 근접공격 모션 ────────────────────────────────────────
	for (int nAttackerId : NETWORK->ConsumePlayerMelees())
	{
		if (nAttackerId == NETWORK->GetPlayerID()) continue; // 본인은 입력으로 이미 재생
		auto it = m_RemotePlayers.find(nAttackerId);
		if (it != m_RemotePlayers.end())
			it->second->PlayMeleeStartAction();
	}

	// ── 좀비 히트: 데미지 + 피 + 히트마커 ────────────────────────────────────
	for (auto& ev : NETWORK->ConsumeMeleeHits())
	{
		auto it = m_ServerZombies.find(ev.zombieId);
		if (it != m_ServerZombies.end() && it->second)
		{
			it->second->TakeDamage(ev.damage);

			ParticleEffectSpawnDesc desc{};
			desc.v3Position = ev.v3HitPoint;
			desc.v3Direction = Vector3::Up;
			desc.v3Normal = Vector3::Up;
			desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
			PARTICLE->Spawn<BloodEffect>(desc);
		}

		// 히트마커 (내 근접공격일 때만)
		if (pPlayer && ev.attackerPlayerId == NETWORK->GetPlayerID())
			pPlayer->ShowHitMarker();
	}
}

void GameScene::ProcessPlayerBandage()
{
	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer)
		return;

	// 타인 회복 캐스트 중 대상 감시 — 이탈(멀어짐)/사망/퇴장 시 취소
	if (pPlayer->IsBandaging())
	{
		const int nTargetId = pPlayer->GetBandageTargetId();
		if (nTargetId >= 0 && nTargetId != NETWORK->GetPlayerID())
		{
			auto it = m_RemotePlayers.find(nTargetId);
			bool bBreak = (it == m_RemotePlayers.end()) || m_DeadPlayers.count(nTargetId);
			if (!bBreak)
			{
				const float fDistSq = Vector3::DistanceSquared(
					pPlayer->GetTransform()->GetPosition(),
					it->second->GetTransform()->GetPosition());
				bBreak = fDistSq > BANDAGE_BREAK_RANGE * BANDAGE_BREAK_RANGE;
			}
			if (bBreak)
				pPlayer->CancelBandage();
		}
	}

	const int nRequest = pPlayer->ConsumeBandageRequest();
	if (nRequest == 0)
		return;

	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();

	// 1 = 자기 회복 (좌클릭)
	if (nRequest == 1)
	{
		if (pPlayer->GetHP() >= pPlayer->GetMaxHP())
			return;	// 풀피면 낭비 방지
		pPlayer->StartBandageCast(bOnline ? NETWORK->GetPlayerID() : -1);
		return;
	}

	// 2 = 타인 회복 (우클릭) — 반경 내 + 전방 ±60° 이내 가장 가까운 살아있는 리모트 (온라인 전용)
	if (!bOnline)
		return;

	Vector3 v3Forward = Vector3::Zero;
	if (auto pCamera = std::dynamic_pointer_cast<ThirdPersonCamera>(pPlayer->GetCamera()))
		v3Forward = pCamera->GetForwardXZ();
	if (v3Forward.LengthSquared() > 1e-6f)
		v3Forward.Normalize();

	const Vector3 v3MyPos = pPlayer->GetTransform()->GetPosition();
	int nBestId = -1;
	float fBestDistSq = BANDAGE_TARGET_RANGE * BANDAGE_TARGET_RANGE;
	for (const auto& [nId, pRemote] : m_RemotePlayers)
	{
		if (!pRemote || m_DeadPlayers.count(nId))
			continue;

		Vector3 v3To = pRemote->GetTransform()->GetPosition() - v3MyPos;
		v3To.y = 0.f;
		const float fDistSq = v3To.LengthSquared();
		if (fDistSq >= fBestDistSq)
			continue;

		// 시야각 체크 — 바라보고 있는 아군만. 겹칠 정도로 붙어있으면(방향 무의미) 통과
		if (fDistSq > 1.f && v3Forward.LengthSquared() > 1e-6f)
		{
			Vector3 v3Dir = v3To / std::sqrt(fDistSq);
			if (v3Forward.Dot(v3Dir) < BANDAGE_TARGET_FOV_COS)
				continue;
		}

		fBestDistSq = fDistSq;
		nBestId = nId;
	}

	if (nBestId >= 0)
		pPlayer->StartBandageCast(nBestId);
}

void GameScene::ProcessBandageResults()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	// ── 리모트 플레이어 붕대 모션 (캐스트 시작/종료 + 들기/내리기) ──────────
	for (auto& ev : NETWORK->ConsumePlayerBandages())
	{
		if (ev.playerId == NETWORK->GetPlayerID()) continue; // 본인은 입력으로 이미 재생
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it == m_RemotePlayers.end()) continue;

		switch (ev.state)
		{
		case 0:	// 캐스트 시작 — 대상이 시전자 본인이 아니면 아군 회복 모션
			it->second->PlayBandageStartAction(
				ev.targetPlayerId >= 0 && ev.targetPlayerId != ev.playerId);
			break;
		case 3:  it->second->PlayBandageHoldAction();   break;	// 들기 (총 내림 + 꺼내는 모션)
		case 4:  it->second->PlayBandageUnholdAction(); break;	// 내리기 (총 복귀 + 꺼내는 모션)
		default: it->second->StopBandageAction();       break;	// 1=취소, 2=완료 → 모션 종료
		}
	}

	// ── 회복 확정 — 대상이 본인이면 서버 권위 HP 반영 ───────────────────────
	for (auto& ev : NETWORK->ConsumePlayerHeals())
	{
		if (ev.targetPlayerId == NETWORK->GetPlayerID() && m_pPlayer)
			m_pPlayer->SetHP(ev.fNewHP);
	}
}

bool GameScene::ComputeGrenadeThrowParams(Vector3& outOrigin, Vector3& outVelocity) const
{
	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer)
		return false;

	// 투척 시작점 — 캡슐 중심 + 머리 위 보정 (궤도 라인이 몸에서 시작하면 어색)
	Vector3 v3Origin = pPlayer->GetTransform()->GetPosition();
	if (auto pSelfCollider = pPlayer->GetComponent<PlayerCollider>())
		v3Origin = pSelfCollider->GetCapsuleWorld().v3Center;
	v3Origin.y += 100.f;

	// 조준 방향 = 카메라 시선
	Vector3 v3Dir = Vector3{ 0.f, 0.f, 1.f };
	if (auto pCamera = std::dynamic_pointer_cast<ThirdPersonCamera>(pPlayer->GetCamera()))
		v3Dir = pCamera->GetLook();
	if (v3Dir.LengthSquared() > 1e-6f)
		v3Dir.Normalize();

	outOrigin   = v3Origin + v3Dir * 50.f;	// 몸통과 겹침 방지
	outVelocity = v3Dir * GRENADE_THROW_SPEED + Vector3{ 0.f, GRENADE_THROW_UPWARD, 0.f };
	return true;
}

void GameScene::ProcessPlayerGrenade()
{
	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer || !pPlayer->ConsumeGrenadeRelease())
		return;

	// 와인드업 중 조준(궤도)한 초기조건 우선 — 표시한 궤도 그대로 날아가게 한다
	Vector3 v3Origin, v3Velocity;
	if (m_bAimedThrowValid) {
		v3Origin   = m_v3AimedThrowOrigin;
		v3Velocity = m_v3AimedThrowVel;
		m_bAimedThrowValid = false;
	}
	else if (!ComputeGrenadeThrowParams(v3Origin, v3Velocity)) {
		return;
	}

	const bool bDecoy = pPlayer->IsDecoySelected();
	SpawnGrenade(v3Origin, v3Velocity, true, bDecoy);

	// 온라인: 리모트 클라가 동일 초기조건으로 투사체를 시뮬하도록 투척 전송
	if (NETWORK->IsConnected() && !NETWORK->IsOffline())
		NETWORK->SendPlayerGrenade(0, v3Origin, v3Velocity, bDecoy ? 1 : 0);
}

void GameScene::UpdateGrenadeArcPreview()
{
	std::vector<Vector3> v3Segments;

	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	const bool bShow = pPlayer
		&& pPlayer->IsHoldingGrenade()
		&& pPlayer->IsGrenadeWindup()	// 좌클릭 꾹(와인드업) 동안만 궤도 표시
		&& !pPlayer->IsGrenadeThrowing()
		&& !IsCinematicActive();

	if (bShow)
	{
		Vector3 v3Origin, v3Velocity;
		if (ComputeGrenadeThrowParams(v3Origin, v3Velocity))
		{
			// 릴리즈 시 이 초기조건 그대로 투척 (ProcessPlayerGrenade가 소비)
			m_v3AimedThrowOrigin = v3Origin;
			m_v3AimedThrowVel    = v3Velocity;
			m_bAimedThrowValid   = true;

			// 단순 포물선 — 충돌/바운스 계산 없이 중력만 적분한 대략적 궤적.
			// 던진 사람 발 높이 아래(평지 가정) 또는 지형에 닿으면 종료.
			constexpr float GRAVITY_Y   = -980.f;		// GrenadeProjectile::GRAVITY와 동일
			constexpr float STEP        = 1.f / 30.f;
			constexpr float MAX_SECONDS = 2.5f;
			const float fStopY = v3Origin.y - 230.f;	// 머리 위 시작점 → 발 높이 근사

			std::vector<Vector3> v3Points;
			Vector3 v3P = v3Origin;
			Vector3 v3V = v3Velocity;
			v3Points.push_back(v3P);
			for (float fT = 0.f; fT < MAX_SECONDS; fT += STEP) {
				v3V.y += GRAVITY_Y * STEP;
				v3P   += v3V * STEP;
				v3Points.push_back(v3P);

				if (v3P.y <= fStopY) break;
				if (QueryTerrainHit(v3P).bGrounded) break;
			}

			// ── 배그식 점선 리본 — 카메라를 향하는 대시 쿼드 (1px 라인은 시선 방향에서 안 보임) ──
			constexpr float DASH_LEN = 30.f;	// 대시 길이 (cm)
			constexpr float GAP_LEN  = 20.f;	// 간격 (cm)
			constexpr float HALF_W   = 3.f;		// 리본 반폭 (cm)

			const Vector3 v3CamPos = m_pMainCamera ? m_pMainCamera->GetPosition() : v3Origin;

			auto EmitQuad = [&](const Vector3& a, const Vector3& b) {
				Vector3 v3Dir = b - a;
				if (v3Dir.LengthSquared() < 1e-4f) return;
				v3Dir.Normalize();

				Vector3 v3ToCam = v3CamPos - (a + b) * 0.5f;
				Vector3 v3Side = v3Dir.Cross(v3ToCam);
				if (v3Side.LengthSquared() < 1e-4f) v3Side = Vector3::Up;
				v3Side.Normalize();
				v3Side *= HALF_W;

				v3Segments.push_back(a - v3Side); v3Segments.push_back(a + v3Side); v3Segments.push_back(b + v3Side);
				v3Segments.push_back(a - v3Side); v3Segments.push_back(b + v3Side); v3Segments.push_back(b - v3Side);
			};

			// 경로 호 길이 기준으로 대시/간격 반복 배치
			float fPattern = 0.f;
			for (size_t i = 0; i + 1 < v3Points.size(); ++i) {
				const Vector3 a = v3Points[i];
				const Vector3 b = v3Points[i + 1];
				const float fSegLen = Vector3::Distance(a, b);
				if (fSegLen < 1e-3f) continue;
				const Vector3 v3SegDir = (b - a) / fSegLen;

				float t = 0.f;
				while (t < fSegLen) {
					constexpr float CYCLE = DASH_LEN + GAP_LEN;
					const float fInCycle = std::fmod(fPattern, CYCLE);
					if (fInCycle < DASH_LEN) {
						const float fDraw = std::min(DASH_LEN - fInCycle, fSegLen - t);
						EmitQuad(a + v3SegDir * t, a + v3SegDir * (t + fDraw));
						t += fDraw; fPattern += fDraw;
					}
					else {
						const float fSkip = std::min(CYCLE - fInCycle, fSegLen - t);
						t += fSkip; fPattern += fSkip;
					}
				}
			}

			// ── 착탄(정지) 지점 원형 마커 — 지면 평면 링 ──────────────────────
			if (!v3Points.empty()) {
				constexpr float RING_R    = 40.f;	// 링 반지름 (cm)
				constexpr float RING_W    = 5.f;	// 링 두께 (cm)
				constexpr int   RING_SEGS = 24;

				const Vector3 v3End = v3Points.back();
				const float fY = v3End.y + 3.f;		// z-fighting 방지
				for (int i = 0; i < RING_SEGS; ++i) {
					const float a0 = XM_2PI * i / RING_SEGS;
					const float a1 = XM_2PI * (i + 1) / RING_SEGS;
					const Vector3 o0{ v3End.x + cosf(a0) * RING_R, fY, v3End.z + sinf(a0) * RING_R };
					const Vector3 o1{ v3End.x + cosf(a1) * RING_R, fY, v3End.z + sinf(a1) * RING_R };
					const Vector3 i0{ v3End.x + cosf(a0) * (RING_R - RING_W), fY, v3End.z + sinf(a0) * (RING_R - RING_W) };
					const Vector3 i1{ v3End.x + cosf(a1) * (RING_R - RING_W), fY, v3End.z + sinf(a1) * (RING_R - RING_W) };

					v3Segments.push_back(i0); v3Segments.push_back(o0); v3Segments.push_back(o1);
					v3Segments.push_back(i0); v3Segments.push_back(o1); v3Segments.push_back(i1);
				}
			}
		}
	}

	// 모드 이탈(던지는 중 제외) 시 조준값 무효화 — 다음 투척에 낡은 방향 사용 방지
	if (pPlayer && !pPlayer->IsHoldingGrenade() && !pPlayer->IsGrenadeThrowing()) {
		m_bAimedThrowValid = false;
	}

	GrenadeArcPass::SetArcVertices(std::move(v3Segments));	// 빈 벡터 = 숨김
}

void GameScene::SpawnGrenade(const Vector3& position, const Vector3& velocity, bool bLocallyOwned, bool bDecoy)
{
	auto pGrenade = std::make_shared<GrenadeProjectile>();
	pGrenade->Initialize();
	pGrenade->SetLocallyOwned(bLocallyOwned);
	pGrenade->SetDecoy(bDecoy);
	pGrenade->Launch(position, velocity);

	AddObject(pGrenade);
	m_ActiveGrenades.push_back(pGrenade);
}

void GameScene::ProcessGrenadeResults()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	// ── 리모트 수류탄 모션/투척 재현 ─────────────────────────────────────────
	for (auto& ev : NETWORK->ConsumePlayerGrenades())
	{
		if (ev.playerId == NETWORK->GetPlayerID()) continue; // 본인은 입력으로 이미 재생
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it == m_RemotePlayers.end()) continue;

		switch (ev.state)
		{
		case 0:	// 투척 — 던지기 모션 + 동일 초기조건 투사체 (시각 전용, 데미지 판정 없음)
			it->second->PlayGrenadeThrowAction();
			SpawnGrenade(ev.v3Pos, ev.v3Vel, false, ev.grenadeType == 1);
			break;
		case 2:  it->second->PlayGrenadeWindupAction();  break;	// 와인드업 (뒤로 들기)
		case 3:  it->second->PlayGrenadeEquipAction();   break;	// 들기 (총 내림)
		case 4:  it->second->PlayGrenadeUnequipAction(); break;	// 내리기 (총 복귀)
		default: break;
		}
	}

	// ── 폭발 좀비 히트 확정 (서버 권위) — 클라 좀비 HP/사망 반영 ─────────────
	for (auto& ev : NETWORK->ConsumeGrenadeHits())
	{
		auto it = m_ServerZombies.find(ev.zombieId);
		if (it != m_ServerZombies.end() && it->second)
			it->second->TakeDamage(ev.damage);
	}
}

void GameScene::UpdateGrenades()
{
	if (m_ActiveGrenades.empty()) return;

	const bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();

	for (auto it = m_ActiveGrenades.begin(); it != m_ActiveGrenades.end(); )
	{
		auto& pGrenade = *it;
		if (!pGrenade || !pGrenade->IsExploded())
		{
			++it;
			continue;
		}

		const Vector3 v3Pos = pGrenade->GetPosition();

		// 폭발 연출 — 수류탄 전용 소형 폭발 (각 클라이언트 로컬 퓨즈로 재생)
		if (m_pEventSequence)
			m_pEventSequence->AddEvent(std::make_shared<GrenadeExplosionEvent>(v3Pos));

		// 판정은 던진 쪽만 — 리모트 시뮬 투사체는 연출 전용
		if (pGrenade->IsLocallyOwned())
		{
			if (pGrenade->IsDecoy())
			{
				// 디코이: 데미지 없음 — 반경 내 좀비를 폭발 지점으로 10초간 유인
				if (bOnline)
					NETWORK->SendGrenadeExplode(v3Pos, 1);	// 서버가 SpreadDistraction 수행
				else
					AI->SpreadDistraction(v3Pos,
						GrenadeProjectile::DECOY_AGGRO_RADIUS,
						GrenadeProjectile::DECOY_AGGRO_DURATION);
			}
			else if (bOnline)
			{
				NETWORK->SendGrenadeExplode(v3Pos, 0);	// 서버 AoE 판정 → S2C_GRENADE_HIT
			}
			else
			{
				// 오프라인: 반경 내 좀비에게 거리 선형 감쇠 데미지
				for (const auto& pZombie : m_World.GetObjects<Zombie>())
				{
					if (!pZombie || !pZombie->IsPoolActive())
						continue;

					const float fDist = Vector3::Distance(
						pZombie->GetTransform()->GetPosition(), v3Pos);
					if (fDist > GrenadeProjectile::EXPLODE_RADIUS)
						continue;

					const float fDamage = GrenadeProjectile::DAMAGE_MAX
						+ (GrenadeProjectile::DAMAGE_MIN - GrenadeProjectile::DAMAGE_MAX)
						* (fDist / GrenadeProjectile::EXPLODE_RADIUS);
					pZombie->TakeDamage(fDamage);
				}
			}
		}

		RemoveObject(pGrenade);
		it = m_ActiveGrenades.erase(it);
	}
}

void GameScene::RemoveDeadZombies()
{
	m_World.RemoveIfAlive<Zombie>([&](const std::shared_ptr<IGameObject>& obj) {
		auto pZombie = std::dynamic_pointer_cast<Zombie>(obj);
		if (!pZombie || !pZombie->IsReadyToRemove()) {
			return false;
		}

		RemoveCollisionPairsOf(pZombie.get());

		// 온라인 좀비면 서버 맵에서도 제거
		int nServerId = pZombie->GetServerId();
		if (nServerId >= 0)
			m_ServerZombies.erase(nServerId);

		return true;
		});

	// 죽은 좀비를 풀로 반환 (swap-with-last O(1), 소멸자 호출 없음)
	m_ZombiePool.Collect();
}

void GameScene::SpawnZombie()
{
	auto pZombie = m_ZombiePool.Acquire();
	if (!pZombie) return; // 풀 고갈

	pZombie->Initialize();
	pZombie->SetFast(RandomGenerator::GenerateRandomIntInRange(0, 99) < Zombie::FAST_ZOMBIE_PERCENT);
	AddObject(pZombie);

	pZombie->SetPosition(AI->GetNavMesh()->GetRandomPoint());
	pZombie->SetTarget(m_pPlayer);
}

// 오프라인 드립 스포너: 일정 간격마다 스폰 포인트 중 랜덤 하나에 좀비 1마리 배치.
// 동시 존재 수가 최대치 미만일 때만. 기본 상태는 AI DLL이 Idle로 시작.
void GameScene::UpdateOfflineSpawner()
{
	// 온라인은 서버가 스폰 — 오프라인에서만 동작
	if (NETWORK->IsConnected() && !NETWORK->IsOffline())
		return;

	const auto& points = GetZombieSpawnPoints();
	if (points.empty())
		return; // 스폰 포인트 없으면 스폰 안 함

	if (m_ZombiePool.GetActiveCount() >= OFFLINE_MAX_ZOMBIES)
		return; // 최대치 도달

	m_fOfflineSpawnTimer += DT;
	if (m_fOfflineSpawnTimer < OFFLINE_SPAWN_INTERVAL)
		return;
	m_fOfflineSpawnTimer = 0.f;

	auto pZombie = m_ZombiePool.Acquire();
	if (!pZombie)
		return; // 풀 고갈

	const Vector3& v3Spawn = points[rand() % points.size()]; // 랜덤 타겟포인트

	pZombie->Initialize();
	pZombie->SetServerId(-1);       // 로컬(오프라인) 좀비
	pZombie->SetFast(RandomGenerator::GenerateRandomIntInRange(0, 99) < Zombie::FAST_ZOMBIE_PERCENT);
	pZombie->SetPosition(v3Spawn);
	pZombie->SetTarget(m_pPlayer);

	// world matrix 즉시 계산 → AddObject의 Spatial 등록 시 올바른 바운드
	pZombie->GetTransform()->Update();
	if (auto pCol = pZombie->GetComponent<PlayerCollider>())
		pCol->Update();

	AddObject(pZombie);
}

void GameScene::ProcessNetworkZombies()
{
	// 오프라인이거나 미연결 시 서버 이벤트 처리 생략
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	// ── 스폰 이벤트 처리 (Task 6) ─────────────────────────────────────────
	for (auto& ev : NETWORK->ConsumeSpawnEvents())
	{
		// 이미 해당 ID로 스폰된 좀비가 있으면 무시
		if (m_ServerZombies.count(ev.zombieId)) continue;

		auto pZombie = m_ZombiePool.Acquire();
		if (!pZombie) break; // 풀 고갈

		pZombie->Initialize();

		pZombie->SetServerId(ev.zombieId);
		pZombie->SetFast(ev.bFast);		// 서버가 롤한 타입 (빠른 좀비 애니 전환용)
		pZombie->SetPosition(ev.pos);
		pZombie->SetTarget(m_pPlayer);

		// world matrix 즉시 계산 → AddObject의 Spatial 등록 시 올바른 바운드
		pZombie->GetTransform()->Update();
		if (auto pCol = pZombie->GetComponent<PlayerCollider>())
			pCol->Update();

		AddObject(pZombie);
		m_ServerZombies[ev.zombieId] = pZombie;
	}

	// ── 디스폰 이벤트 처리 (Task 6) ──────────────────────────────────────
	for (int nId : NETWORK->ConsumeDespawnEvents())
	{
		auto it = m_ServerZombies.find(nId);
		if (it == m_ServerZombies.end()) continue;

		auto pZombie = it->second;       // erase 전에 복사
		m_ZombiePool.MarkForRelease(pZombie);
		m_ServerZombies.erase(it);
		//RemoveObject(pZombie);
	}

	// ── 좀비 상태 적용 (Task 7) ───────────────────────────────────────────
	for (auto& [nId, pZombie] : m_ServerZombies)
	{
		if (!pZombie || !pZombie->IsPoolActive()) continue;

		ZombieServerState state;
		if (NETWORK->GetLatestZombieState(nId, state))
			pZombie->ApplyServerState(state.x, state.z, state.yaw,
				state.behaviorState, state.receivedTime);
	}

	// ── 공격 이벤트 처리 (Task 9) ─────────────────────────────────────────
	for (auto& ev : NETWORK->ConsumeAttackEvents())
	{
		// damage=0: 모션 시작 이벤트 (몽타주만 재생)
		// damage>0: 데미지 발동 이벤트 (Notify 딜레이 후)
		if (ev.damage <= 0.f)
		{
			// 공격 몽타주는 모든 클라이언트에서 재생
			auto it = m_ServerZombies.find(ev.zombieId);
			if (it != m_ServerZombies.end() && it->second)
			{
				auto pAC = it->second->GetComponent<ZombieAnimationController>();
				if (pAC && pAC->GetMontage())
					pAC->GetMontage()->PlayMontage(ev.animIndex == 1 ? "Zombie Attack1" : "Zombie Attack");
			}
		}
		else
		{
			// 데미지는 나 자신이 타겟인 경우에만 적용 (이미 죽어있으면 무시)
			if (ev.targetPlayerId == NETWORK->GetPlayerID() && !m_pPlayer->IsDead())
				m_pPlayer->TakeDamage(ev.damage);
		}
	}
}

void GameScene::ProcessShootResults()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	auto pPlayer = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer);

	for (auto& ev : NETWORK->ConsumeShootResults())
	{
		// ── 벽 히트: BulletImpactEffect (먼지) ──────────────────────────────
		if (ev.bHit == 1)
		{
			Vector3 v3Normal = ev.v3HitNormal;
			v3Normal.Normalize();

			ParticleEffectSpawnDesc desc{};
			desc.v3Position = ev.v3HitPoint;
			desc.v3Direction = v3Normal;
			desc.v3Normal = v3Normal;
			desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, Vector3::Up);

			PARTICLE->Spawn<BulletImpactEffect>(desc);
		}

		// ── 좀비 히트: BloodEffect + 데미지 + HitMarker ─────────────────────
		if (ev.bHit == 2 && ev.hitZombieId >= 0)
		{
			auto it = m_ServerZombies.find(ev.hitZombieId);
			if (it != m_ServerZombies.end() && it->second)
			{
				auto& pZombie = it->second;

				// 데미지 적용 (→ PostUpdate에서 IsDead() → 사망 몽타주 → 풀 반환)
				pZombie->TakeDamage(ev.damage);

				// 피 이펙트
				Vector3 v3Dir = ev.v3HitNormal;
				if (v3Dir.LengthSquared() > 1e-8f)
					v3Dir.Normalize();

				ParticleEffectSpawnDesc desc{};
				desc.v3Position = ev.v3HitPoint;
				desc.v3Direction = -v3Dir;
				desc.v3Normal = ev.v3HitNormal;
				desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);

				PARTICLE->Spawn<BloodEffect>(desc);
			}

			// 히트마커 (내가 쏜 총인 경우만)
			if (pPlayer && ev.shooterPlayerId == NETWORK->GetPlayerID())
				pPlayer->ShowHitMarker();
		}

		// ── 다른 플레이어의 총구 이펙트 + 발사 애니메이션 ────────────────────
		if (ev.shooterPlayerId != NETWORK->GetPlayerID())
		{
			auto it = m_RemotePlayers.find(ev.shooterPlayerId);
			if (it != m_RemotePlayers.end()) {
				it->second->PlayFireAction();
				if (auto pWeapon = it->second->GetCurrentWeaponObject()) {
					pWeapon->PlayFireSound();
				}
			}

			Vector3 v3MuzzleDir = ev.v3ShootDir;
			if (v3MuzzleDir.LengthSquared() > 1e-8f)
				v3MuzzleDir.Normalize();

			ParticleEffectSpawnDesc desc{};
			desc.v3Position = ev.v3MuzzlePos;
			desc.v3Direction = v3MuzzleDir;
			desc.v3Normal = Vector3::Up;
			desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);

			PARTICLE->Spawn<MuzzleFlashEffect>(desc);
		}
	}
}

void GameScene::SyncSceneWithServer()
{
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;
	for (auto& [id, player] : m_RemotePlayers) {
		if (auto remote = std::dynamic_pointer_cast<NetworkRemoteThirdPersonPlayer>(player)) {
			if (TIME->GetTotalTime() - remote->GetLastPacketTime() > 0.3f) {
				remote->ResetMovementState();
			}
		}
	}

	// 큐(ConsumePlayerJoins)가 아니라 방 멤버 스냅샷으로 생성한다.
	// 로비가 입장 큐를 이미 소비했어도 스냅샷은 유지되므로 게임씬에서 리모트가 누락되지 않는다.
	for (auto& ev : NETWORK->GetRoomPlayersSnapshot()) {

		if (ev.playerId == NETWORK->GetPlayerID()) continue; // 본인 제외
		if (m_RemotePlayers.contains(ev.playerId)) continue;

		auto remotePlayer = std::make_shared<NetworkRemoteThirdPersonPlayer>();
		remotePlayer->Initialize();

		remotePlayer->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.initialTransform.m), ev.bRunning, ev.bAiming, ev.fAimPitch, NetworkManager::GetNetTimeSec());
		remotePlayer->GiveWeapon(static_cast<WEAPON_TYPE>(ev.weaponType));
		if (ev.characterType < GameContext::g_unCharacterModels)
			remotePlayer->SetPlayerModel(GameContext::g_strCharacterNames[ev.characterType]);
		AddObject(remotePlayer);
		m_RemotePlayers[ev.playerId] = remotePlayer;
		m_RemoteSpawnTimes[ev.playerId] = NetworkManager::GetNetTimeSec();
	}

	for (auto id : NETWORK->ConsumePlayerLeaves()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			RemoveObject(it->second);
			m_RemotePlayers.erase(it);
		}
		// 서버가 슬롯 id를 재사용하므로 사망 기록도 함께 제거 — 안 하면 같은 id로
		// 들어온 새 플레이어가 관전 후보에서 영구 제외된다.
		m_DeadPlayers.erase(id);
		m_RemoteSpawnTimes.erase(id);
	}

	for (auto& ev : NETWORK->ConsumePlayerTransforms()) {
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end()) {
			it->second->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.transform.m), ev.bRunning, ev.bAiming, ev.fAimPitch, ev.fRecvTime);
		}
	}

	for (auto id : NETWORK->ConsumePlayerReloads()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			it->second->PlayReloadStartAction();
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerWeapons()) {
		if (ev.playerId == NETWORK->GetPlayerID())
			continue; // 본인은 입력으로 이미 교체
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end()) {
			const auto eType = static_cast<WEAPON_TYPE>(ev.weaponType);
			if (it->second->GetCurrentWeaponType() == eType)
				continue; // 이미 같은 무기 — 꺼내기 모션 생략
			it->second->GiveWeapon(eType);

			// 스폰 직후 유예 시간 내 도착한 이벤트는 게임 시작 초기 장착 동기화
			// (로비 마지막 브로드캐스트가 무기2였으면 스냅샷과 어긋날 수 있음) —
			// 무기만 갈아끼우고 꺼내기 모션은 생략
			auto itSpawn = m_RemoteSpawnTimes.find(ev.playerId);
			const bool bJustSpawned = itSpawn != m_RemoteSpawnTimes.end()
				&& NetworkManager::GetNetTimeSec() - itSpawn->second < 1.f;
			if (!bJustSpawned)
				it->second->PlayWeaponDrawAction();	// 리모트도 꺼내기 모션 동기화
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerCharacters()) {
		if (ev.playerId == NETWORK->GetPlayerID())
			continue; // 본인 모델은 이미 적용됨
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end() && ev.characterType < GameContext::g_unCharacterModels) {
			it->second->SetPlayerModel(GameContext::g_strCharacterNames[ev.characterType]);
		}
	}
}
