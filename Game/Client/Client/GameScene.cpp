#include "pch.h"
#include "GameScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "TerrainObject.h"
#include "TerrainTestScene.h"
#include "Skybox.h"
#include "MapTestScene.h"
#include "TextBox.h"
#include "BloodEffect.h"
#include "MuzzleFlashEffect.h"
#include "ZombieAnimationController.h"
#include "HelicopterAnimationController.h"
#include "ThirdPersonCamera.h"
#include "BulletImpactEffect.h"
#include "EventSequence.h"

void GameScene::BuildObjects()
{
	using namespace std::chrono;

	m_pUIBoard = std::make_unique<UIBoard>();
	m_pPlayer = std::make_shared<NetworkOwnerThirdPersonPlayer>();

	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();
	m_pPlayer->Initialize();
	m_pPlayer->GetTransform()->SetPosition(29000, -3536.692724, 25000);
	if (auto pThirdPerson = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		const auto& data = GCTX->GetGameData();
		pThirdPerson->SetPlayerModel(GameContext::g_strCharacterNames[data.m_nCurModelIndex]);
		pThirdPerson->GiveWeapon((WEAPON_TYPE)data.m_nCurWeaponIndex);
		if (bOnline)
			NETWORK->SendPlayerCharacter(static_cast<unsigned char>(data.m_nCurModelIndex)); // late-join 리모트에 모델 반영
	}


	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();
	m_pSkybox->LoadSkyboxParameters("Day");

	//m_pPlayer = std::make_shared<DebugPlayer>();
	//m_pPlayer->Initialize();

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

	//std::shared_ptr<TextBox> pText = std::make_shared<TextBox>(L"Malgun Gothic");
	//pText->SetText(std::format(L"로딩 시간 : {}ms", llLoadTime));
	//pText->SetLayer(0);
	//pText->SetAnchor(Vector2{ 0,0 });
	//pText->SetPivot(Vector2{ 0,0 });
	//pText->SetPosition(Vector2{ 10,150 });
	//pText->SetTextHeight(50);
	//m_pUIBoard->InsertUI(pText);

	BuildChatUI();

	// 탈출 시퀀스 안내 HUD (총알/체력 UI와 동일하게 TextBox → UIBoard). 평소엔 숨김.
	m_pEscapeText = std::make_shared<TextBox>(L"Malgun Gothic");
	m_pEscapeText->SetText(L"");
	m_pEscapeText->SetLayer(0);
	m_pEscapeText->SetAnchor(Vector2{ 0.5f, 0.f });   // 화면 상단 중앙
	m_pEscapeText->SetPivot(Vector2{ 0.5f, 0.f });
	m_pEscapeText->SetPosition(Vector2{ 0.f, 60.f });
	m_pEscapeText->SetTextHeight(60.f);
	m_pEscapeText->SetVisible(false);
	m_pUIBoard->InsertUI(m_pEscapeText);

	// 서버 트리거 게임 이벤트용 시퀀스 (런타임에 이벤트 AddEvent).
	// Scene::FixedUpdate()가 매 프레임 m_pEventSequence->Update() 호출.
	m_pEventSequence = std::make_shared<EventSequence>(this);

	m_pWater = std::make_shared<WaterGridObject>();
	//pWater->Initialize();

	auto& pTransform = m_pWater->GetTransform();
	m_v3WaterPos = Vector3(0.f, -47_m, 0.f);
	pTransform->SetPosition(m_v3WaterPos);
	AddObject(m_pWater);

	// ── [임시 디버그] 시작 지점에 헬기 1대 고정 소환 (회전 보정 없이 원본 방향) ──
	{
		auto pHeli = MODEL->LoadOrGet("Gunship", true)->CopyObject<CrashDebris>();
		pHeli->Initialize();
		pHeli->AddComponent<HelicopterAnimationController>();
		pHeli->GetComponent<AnimationController>()->Initialize();
		pHeli->GetTransform()->SetFrameMatrix(Matrix::Identity);

		const Vector3 v3Pos{ 10000, -3536.692724, 20000 };       // 플레이어 시작 앞쪽, 살짝 위
		pHeli->GetTransform()->SetWorldMatrix(Matrix::CreateTranslation(v3Pos));
		pHeli->GetTransform()->Update();
		AddObject(pHeli);
	}

}

void GameScene::OnEnterScene()
{
}

void GameScene::OnLeaveScene()
{
}

void GameScene::ProcessInput()
{
}

void GameScene::Update()
{
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

	// ====== Sound Test ======
	ImGui::Begin("Sound Test");
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
	}
	ImGui::End();

	UpdateOfflineSpawner();
	SyncSceneWithServer();
	ProcessNetworkZombies();
	ProcessPlayerMelee();
	ProcessShootResults();
	ProcessMeleeResults();
	ProcessServerGameEvents();
	UpdateEscapeSequence();
	RemoveDeadZombies();
	UpdateChat();
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
		std::shared_ptr<CrashDebris> pHeli = m_pArriveEvent ? m_pArriveEvent->GetHeli() : nullptr;
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
			if (!m_bEscapeKeySent && INPUT->GetButtonDown('F')) {
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
	m_pChatInput = std::make_shared<InputTextBox>(L"Malgun Gothic");
	m_pChatInput->SetPlaceholder(L"Press Enter to chat");
	m_pChatInput->SetLayer(0);
	m_pChatInput->SetAnchor(Vector2{ 0.f, 1.f });
	m_pChatInput->SetPivot(Vector2{ 0.f, 1.f });
	m_pChatInput->SetPosition(Vector2{ fLeft, -20.f });
	m_pChatInput->SetTextHeight(fInputH);
	m_pUIBoard->InsertUI(m_pChatInput);

	// 히스토리 줄: 입력창 위로 쌓임. 인덱스 0 = 맨 아래(최신), 클수록 과거
	for (int i = 0; i < CHAT_VISIBLE_LINES; ++i) {
		auto pLine = std::make_shared<TextBox>(L"Malgun Gothic");
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
	if (INPUT->GetButtonDown(VK_RETURN)) {
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
					pAC->GetMontage()->PlayMontage("Zombie Attack");
			}
		}
		else
		{
			// 데미지는 나 자신이 타겟인 경우에만 적용
			if (ev.targetPlayerId == NETWORK->GetPlayerID())
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

		remotePlayer->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.initialTransform.m), ev.bRunning, ev.bAiming, ev.fAimPitch);
		remotePlayer->GiveWeapon(static_cast<WEAPON_TYPE>(ev.weaponType));
		if (ev.characterType < GameContext::g_unCharacterModels)
			remotePlayer->SetPlayerModel(GameContext::g_strCharacterNames[ev.characterType]);
		AddObject(remotePlayer);
		m_RemotePlayers[ev.playerId] = remotePlayer;
	}

	for (auto id : NETWORK->ConsumePlayerLeaves()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			RemoveObject(it->second);
			m_RemotePlayers.erase(it);
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerTransforms()) {
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end()) {
			it->second->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.transform.m), ev.bRunning, ev.bAiming, ev.fAimPitch);
		}
	}

	for (auto id : NETWORK->ConsumePlayerReloads()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			std::cout << "[Scene] Playing Reload for Remote Player " << id << std::endl;
			it->second->PlayReloadStartAction();
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerWeapons()) {
		if (ev.playerId == NETWORK->GetPlayerID())
			continue; // 본인은 입력으로 이미 교체
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end()) {
			it->second->GiveWeapon(static_cast<WEAPON_TYPE>(ev.weaponType));
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
