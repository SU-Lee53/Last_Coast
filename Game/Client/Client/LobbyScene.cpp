#include "pch.h"
#include "LobbyScene.h"
#include "DebugPlayer.h"
#include "TextBox.h"
#include "Skybox.h"
#include "GameScene.h"

void LobbyScene::BuildObjects()
{
	m_bEnableGravity = false;

	m_v4GlobalAmbient = { 0.25f, 0.25f, 0.25f, 1.f };
	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();
	m_pSkybox->LoadSkyboxParameters("Lobby");

	m_pPlayer = std::make_shared<NetworkOwnerThirdPersonPlayer>();
	m_pPlayer->Initialize();
	static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)->GiveWeapon((WEAPON_TYPE)m_nWeapon1Index);

	m_pViewCamera = std::make_shared<Camera>();
	m_pViewCamera->SetViewport(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight, 0.f, 1.f);
	m_pViewCamera->SetScissorRect(0, 0, WinCore::g_dwClientWidth, WinCore::g_dwClientHeight);

	const Vector3 v3PreviewCameraPos{ 0.f, 1.5_m, 4.0_m };
	const Vector3 v3PreviewTarget{ 0.f, 1.0_m, 0.f };
	const Vector3 v3PreviewLookDir = v3PreviewTarget - v3PreviewCameraPos;

	m_pViewCamera->GenerateViewMatrix(
		v3PreviewCameraPos,
		v3PreviewLookDir,
		Vector3::Up
	);
	m_pViewCamera->GenerateProjectionMatrix(
		1.f,
		50_m,
		static_cast<float>(WinCore::g_dwClientWidth) / static_cast<float>(WinCore::g_dwClientHeight),
		60.0f
	);
	m_pViewCamera->Update();

	float fWidth = static_cast<float>(WinCore::g_dwClientWidth);
	float fHeight = static_cast<float>(WinCore::g_dwClientHeight);


	// Setup UI
	m_pUIBoard = std::make_unique<UIBoard>();

	{
		// Model Selector
		{
			std::shared_ptr<TextBox> pModelText = std::make_shared<TextBox>(L"Noto Sans KR");
			pModelText->SetText(L"Character : ");
			pModelText->SetLayer(0);
			pModelText->SetAnchor(Vector2{ 0.0, 0.0 });
			pModelText->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
			pModelText->SetPosition(Vector2{ 900,500 });
			pModelText->SetTextHeight(70);
			m_pUIBoard->InsertUI(pModelText);

			std::shared_ptr<TextButton> pModelPrev = std::make_shared<TextButton>(L"Noto Sans KR");
			pModelPrev->SetText(L"<<");
			pModelPrev->SetLayer(0);
			pModelPrev->SetAnchor(Vector2{ 0.0, 0.0 });
			pModelPrev->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
			pModelPrev->SetPosition(Vector2{ 1000,500 });
			pModelPrev->SetTextHeight(70);

			pModelPrev->SetButtonCallback(
				[&](IUIComponent* pComp) {
					const int32 nModelCount = static_cast<int32>(GameContext::g_unCharacterModels);
					m_nCurModelIndex = (m_nCurModelIndex - 1 + nModelCount) % nModelCount;
					if (auto p = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
						p->SetPlayerModel(GameContext::g_strCharacterNames[m_nCurModelIndex]);
						m_pModelNameTextbox->SetText(GameContext::g_strCharacterNames[m_nCurModelIndex]);
						NETWORK->SendPlayerCharacter(static_cast<unsigned char>(m_nCurModelIndex));
					}
				}
			);

			m_pUIBoard->InsertUI(pModelPrev);

			std::shared_ptr<TextButton> pModelNext = std::make_shared<TextButton>(L"Noto Sans KR");
			pModelNext->SetText(L">>");
			pModelNext->SetLayer(0);
			pModelNext->SetAnchor(Vector2{ 0.0, 0.0 });
			pModelNext->SetPivot(Vector2{ 0.0,0.0 });	// Pivot to left
			pModelNext->SetPosition(Vector2{ 1400,500 });
			pModelNext->SetTextHeight(70);

			pModelNext->SetButtonCallback(
				[&](IUIComponent* pComp) {
					m_nCurModelIndex = (m_nCurModelIndex + 1) % static_cast<int32>(GameContext::g_unCharacterModels);
					if (auto p = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
						p->SetPlayerModel(GameContext::g_strCharacterNames[m_nCurModelIndex]);
						m_pModelNameTextbox->SetText(GameContext::g_strCharacterNames[m_nCurModelIndex]);
						NETWORK->SendPlayerCharacter(static_cast<unsigned char>(m_nCurModelIndex));
					}
				}
			);
			m_pUIBoard->InsertUI(pModelNext);

			m_pModelNameTextbox = std::make_shared<TextBox>(L"Noto Sans KR");
			m_pModelNameTextbox->SetText(GameContext::g_strCharacterNames[m_nCurModelIndex]);
			m_pModelNameTextbox->SetLayer(0);
			m_pModelNameTextbox->SetAnchor(Vector2{ 0.0, 0.0 });
			m_pModelNameTextbox->SetPivot(Vector2{ 0.5, 0.0 });
			m_pModelNameTextbox->SetPosition(Vector2{ 1200,500 });
			m_pModelNameTextbox->SetTextHeight(70);

			m_pUIBoard->InsertUI(m_pModelNameTextbox);

		}

		// Weapon Selector — 1/2번 슬롯 주무기 선택 (3번 슬롯은 인게임 권총 고정)
		{
			auto BuildWeaponSelector = [&](const std::wstring& strLabel, float fY,
				int32* pnIndex, std::shared_ptr<TextBox>* ppNameTextbox) {

				std::shared_ptr<TextBox> pWeaponText = std::make_shared<TextBox>(L"Noto Sans KR");
				pWeaponText->SetText(strLabel);
				pWeaponText->SetLayer(0);
				pWeaponText->SetAnchor(Vector2{ 0.0, 0.0 });
				pWeaponText->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
				pWeaponText->SetPosition(Vector2{ 900, fY });
				pWeaponText->SetTextHeight(70);
				m_pUIBoard->InsertUI(pWeaponText);

				// 선택 변경 공통 처리 — 미리보기 무기 교체 + 이름 갱신 + 서버 통지
				auto OnWeaponChanged = [this, pnIndex, ppNameTextbox](int nDelta) {
					const int32 nMainWeaponCount = static_cast<int32>(GameContext::g_unWeapons - 2);
					*pnIndex = (*pnIndex + nDelta + nMainWeaponCount) % nMainWeaponCount;
					if (auto p = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
						p->GiveWeapon((WEAPON_TYPE)*pnIndex);
						(*ppNameTextbox)->SetText(GameContext::g_strWeaponNames[*pnIndex]);
						NETWORK->SendPlayerWeapon(static_cast<unsigned char>(*pnIndex));
					}
				};

				std::shared_ptr<TextButton> pWeaponPrev = std::make_shared<TextButton>(L"Noto Sans KR");
				pWeaponPrev->SetText(L"<<");
				pWeaponPrev->SetLayer(0);
				pWeaponPrev->SetAnchor(Vector2{ 0.0, 0.0 });
				pWeaponPrev->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
				pWeaponPrev->SetPosition(Vector2{ 1000, fY });
				pWeaponPrev->SetTextHeight(70);
				pWeaponPrev->SetButtonCallback(
					[OnWeaponChanged](IUIComponent* pComp) { OnWeaponChanged(-1); });
				m_pUIBoard->InsertUI(pWeaponPrev);

				std::shared_ptr<TextButton> pWeaponNext = std::make_shared<TextButton>(L"Noto Sans KR");
				pWeaponNext->SetText(L">>");
				pWeaponNext->SetLayer(0);
				pWeaponNext->SetAnchor(Vector2{ 0.0, 0.0 });
				pWeaponNext->SetPivot(Vector2{ 0.0,0.0 });	// Pivot to left
				pWeaponNext->SetPosition(Vector2{ 1400, fY });
				pWeaponNext->SetTextHeight(70);
				pWeaponNext->SetButtonCallback(
					[OnWeaponChanged](IUIComponent* pComp) { OnWeaponChanged(+1); });
				m_pUIBoard->InsertUI(pWeaponNext);

				*ppNameTextbox = std::make_shared<TextBox>(L"Noto Sans KR");
				(*ppNameTextbox)->SetText(GameContext::g_strWeaponNames[*pnIndex]);
				(*ppNameTextbox)->SetLayer(0);
				(*ppNameTextbox)->SetAnchor(Vector2{ 0.0, 0.0 });
				(*ppNameTextbox)->SetPivot(Vector2{ 0.5, 0.0 });
				(*ppNameTextbox)->SetPosition(Vector2{ 1200, fY });
				(*ppNameTextbox)->SetTextHeight(70);
				m_pUIBoard->InsertUI(*ppNameTextbox);
			};

			BuildWeaponSelector(L"Weapon 1 : ", 570.f, &m_nWeapon1Index, &m_pWeapon1NameTextbox);
			BuildWeaponSelector(L"Weapon 2 : ", 640.f, &m_nWeapon2Index, &m_pWeapon2NameTextbox);
		}

		// Buttons
		{
			std::shared_ptr<TextButton> pReadyButton = std::make_shared<TextButton>(L"Noto Sans KR");
			pReadyButton->SetText(m_bReadyState ? "Ready" : "Not Ready");
			pReadyButton->SetLayer(0);
			pReadyButton->SetAnchor(Vector2{ 0.0, 0.0 });
			pReadyButton->SetPivot(Vector2{ 0.5, 0.0 });
			pReadyButton->SetPosition(Vector2{ 1000,730 });
			pReadyButton->SetTextHeight(70);

			pReadyButton->SetButtonCallback(
				[&](IUIComponent* pComp) {
					m_bReadyState = !m_bReadyState;
					static_cast<TextButton*>(pComp)->SetText(m_bReadyState ? "Ready" : "Not Ready");
					NETWORK->SendReady(m_bReadyState);
				}
			);

			m_pUIBoard->InsertUI(pReadyButton);

			// 방장 전용 시작 버튼 — 호스트에게만 표시, 서버가 전원 레디 검증
			m_pStartButton = std::make_shared<TextButton>(L"Noto Sans KR");
			m_pStartButton->SetText("Start Game");
			m_pStartButton->SetLayer(0);
			m_pStartButton->SetAnchor(Vector2{ 0.0, 0.0 });
			m_pStartButton->SetPivot(Vector2{ 0.5, 0.0 });
			m_pStartButton->SetPosition(Vector2{ 1000, 820 });
			m_pStartButton->SetTextHeight(70);
			m_pStartButton->SetVisible(false);

			m_pStartButton->SetButtonCallback(
				[&](IUIComponent* pComp) {
					if (NETWORK->IsHost() && m_bAllReady)
						NETWORK->SendGameStart();
				}
			);


			m_pUIBoard->InsertUI(m_pStartButton);

			std::shared_ptr<TextButton> pOfflineButton = std::make_shared<TextButton>(L"Noto Sans KR");
			pOfflineButton->SetText("Test Offline");
			pOfflineButton->SetLayer(0);
			pOfflineButton->SetAnchor(Vector2{ 1.0, 1.0 });
			pOfflineButton->SetPivot(Vector2{ 1.0, 1.0 });
			pOfflineButton->SetPosition(Vector2{ 0,0 });
			pOfflineButton->SetTextHeight(70);

			pOfflineButton->SetButtonCallback(
				[&](IUIComponent* pComp) {
					m_bProceed = true;
				}
			);

			m_pUIBoard->InsertUI(pOfflineButton);
		}
	}

}

void LobbyScene::OnEnterScene()
{
	// 로비 진입 시 본인 캐릭터/무기 선택을 서버에 1회 통지 (이미 접속한 다른 클라에 반영 + 서버 기본값 보정)
	if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
		NETWORK->SendPlayerCharacter(static_cast<unsigned char>(m_nCurModelIndex));
		NETWORK->SendPlayerWeapon(static_cast<unsigned char>(m_nWeapon1Index));
	}
}

void LobbyScene::OnLeaveScene()
{
	GameContext::GameData data;
	data.m_nCurModelIndex = m_nCurModelIndex;
	data.m_nWeapon1Index = m_nWeapon1Index;
	data.m_nWeapon2Index = m_nWeapon2Index;
	GCTX->SetGameData(data);
}

void LobbyScene::ProcessInput()
{
	//auto v3Pos = m_pViewCamera->GetPosition();
	//if (INPUT->GetButtonPressed('W')) {
	//	v3Pos.z += 10.f * DT;
	//}

	//if (INPUT->GetButtonPressed('S')) {
	//	v3Pos.z -= 10.f * DT;
	//}

	//if (INPUT->GetButtonPressed('A')) {
	//	v3Pos.x += 10.f * DT;
	//}

	//if (INPUT->GetButtonPressed('D')) {
	//	v3Pos.x -= 10.f * DT;
	//}
	//m_pViewCamera->SetPosition(v3Pos);

}

void LobbyScene::Update()
{
	if (m_pMainCamera != m_pViewCamera) {
		m_pSwappedPlayerCamera = SwapCamera(m_pViewCamera);
	}

	m_pMainCamera->Update();
	m_pPlayer->GetTransform()->SetPosition(m_v3CurrentPlayerPos);
	m_pPlayer->GetTransform()->SetRotation(m_v3CurrentPlayerOrientation);


	/*ImGui::Begin("Preview");
	for (uint32 i = 0; i < m_PreviewTransforms.size(); ++i) {
		ImGui::DragFloat3(std::format("Pos of {}", i).c_str(), (float*)(&m_PreviewTransforms[i].v3Position), 1.f, -1000.f, 1000.f);
		ImGui::DragFloat3(std::format("Orientation of {}", i).c_str(), (float*)(&m_PreviewTransforms[i].v3Orientation), 1.f, 0.f, 360.f);
		ImGui::NewLine();
	}*/

	for (auto& p : m_ActivePreviews) {
		p.pPlayer->GetTransform()->SetPosition(m_PreviewTransforms[p.nSlotIndex].v3Position);
		p.pPlayer->GetTransform()->SetRotation(m_PreviewTransforms[p.nSlotIndex].v3Orientation);
	}

	ImGui::End();

	if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
		for (auto& ev : NETWORK->ConsumePlayerJoins()) {
			auto it = std::find_if(m_ActivePreviews.begin(), m_ActivePreviews.end(), [&](const LobbyPreviewPlayer& p) { return p.nPlayerId == ev.playerId; });
			if (it != m_ActivePreviews.end()) continue;

			int slot = -1;
			for (int i = 0; i < 3; ++i) {
				bool bUsed = false;
				for (const auto& p : m_ActivePreviews) {
					if (p.nSlotIndex == i) { bUsed = true; break; }
				}
				if (!bUsed) { slot = i; break; }
			}

			if (slot != -1) {
				LobbyPreviewPlayer newPreview;
				newPreview.nPlayerId = ev.playerId;
				newPreview.nSlotIndex = slot;

				newPreview.pPlayer = std::make_shared<NetworkRemoteThirdPersonPlayer>();
				newPreview.pPlayer->Initialize();
				newPreview.pPlayer->GetTransform()->SetPosition(m_PreviewTransforms[slot].v3Position);
				newPreview.pPlayer->GetTransform()->SetRotation(m_PreviewTransforms[slot].v3Orientation);
				newPreview.pPlayer->GiveWeapon((WEAPON_TYPE)ev.weaponType);
				if (ev.characterType < GameContext::g_unCharacterModels)
					newPreview.pPlayer->SetPlayerModel(GameContext::g_strCharacterNames[ev.characterType]);
				AddObject(newPreview.pPlayer);

				newPreview.wstrUsername = ::StringToWString(ev.username);
				newPreview.bReady = ev.bReady;   // 기존 인원의 현재 레디 상태 반영 (late-join)
				newPreview.pNameTag = std::make_shared<TextBox>(L"Noto Sans KR");
				newPreview.pNameTag->SetText(newPreview.wstrUsername + (ev.bReady ? L"  [READY]" : L""));
				if (ev.bReady)
					newPreview.pNameTag->SetColor(Vector3{ 0.4f, 1.0f, 0.4f });
				newPreview.pNameTag->SetLayer(0);
				newPreview.pNameTag->SetAnchor(Vector2{ 0.0f, 0.0f });
				newPreview.pNameTag->SetPivot(Vector2{ 0.5f, 0.5f });
				newPreview.pNameTag->SetTextHeight(30.f);
				m_pUIBoard->InsertUI(newPreview.pNameTag);

				m_ActivePreviews.push_back(newPreview);
			}
		}

		for (auto id : NETWORK->ConsumePlayerLeaves()) {
			auto it = std::find_if(m_ActivePreviews.begin(), m_ActivePreviews.end(), [&](const LobbyPreviewPlayer& p) { return p.nPlayerId == id; });
			if (it != m_ActivePreviews.end()) {
				m_pUIBoard->RemoveUI(it->pNameTag);
				RemoveObject(it->pPlayer);
				m_ActivePreviews.erase(it);
			}
		}

		// 리모트 무기 교체 반영
		for (auto& ev : NETWORK->ConsumePlayerWeapons()) {
			auto it = std::find_if(m_ActivePreviews.begin(), m_ActivePreviews.end(), [&](const LobbyPreviewPlayer& p) { return p.nPlayerId == ev.playerId; });
			if (it != m_ActivePreviews.end())
				it->pPlayer->GiveWeapon((WEAPON_TYPE)ev.weaponType);
		}

		// 리모트 캐릭터 모델 변경 반영
		for (auto& ev : NETWORK->ConsumePlayerCharacters()) {
			auto it = std::find_if(m_ActivePreviews.begin(), m_ActivePreviews.end(), [&](const LobbyPreviewPlayer& p) { return p.nPlayerId == ev.playerId; });
			if (it != m_ActivePreviews.end() && ev.characterType < GameContext::g_unCharacterModels)
				it->pPlayer->SetPlayerModel(GameContext::g_strCharacterNames[ev.characterType]);
		}

		// 방장에게만 시작 버튼 표시 (방장 승계 포함 실시간 갱신)
		if (m_pStartButton) {
			m_pStartButton->SetVisible(NETWORK->IsHost());

			// 전원 레디(본인 + 리모트 전부) 여부
			bool bAllReady = m_bReadyState;
			for (const auto& p : m_ActivePreviews) {
				if (!p.bReady) { bAllReady = false; break; }
			}
			m_bAllReady = bAllReady;

			// 색: 전원 레디 전 = 회색 / 전원 레디 = 흰색 / 전원 레디 + 호버 = 빨간색
			Vector3 v3Color;
			if (!m_bAllReady)
				v3Color = Vector3{ 0.45f, 0.45f, 0.45f };
			else if (m_pStartButton->WasHovered())
				v3Color = Vector3{ 1.0f, 0.2f, 0.2f };
			else
				v3Color = Vector3{ 1.0f, 1.0f, 1.0f };
			m_pStartButton->SetColor(v3Color);
		}

		// 다른 플레이어 레디 상태 변경 → 닉네임 옆에 [READY] 표시 (레디면 초록).
		for (auto& ev : NETWORK->ConsumeReadyStates()) {
			auto it = std::find_if(m_ActivePreviews.begin(), m_ActivePreviews.end(),
				[&](const LobbyPreviewPlayer& p) { return p.nPlayerId == ev.playerId; });
			if (it == m_ActivePreviews.end() || !it->pNameTag) continue;

			it->bReady = ev.bReady;
			it->pNameTag->SetText(it->wstrUsername + (ev.bReady ? L"  [READY]" : L""));
			it->pNameTag->SetColor(ev.bReady ? Vector3{ 0.4f, 1.0f, 0.4f } : Vector3{ 1.0f, 1.0f, 1.0f });
		}

		Matrix mtxVP = m_pViewCamera->GetViewProjectMatrix();
		for (auto& preview : m_ActivePreviews) {
			Vector3 headPos = preview.pPlayer->GetTransform()->GetPosition() + Vector3(0.f, 180.f, 0.f);
			Vector4 clipPos = Vector4::Transform(Vector4(headPos.x, headPos.y, headPos.z, 1.0f), mtxVP);

			if (clipPos.w > 0.01f && clipPos.z > 0.0f) {
				float ndcX = clipPos.x / clipPos.w;
				float ndcY = clipPos.y / clipPos.w;
				float screenX = (ndcX + 1.0f) * 0.5f * WinCore::g_dwClientWidth;
				float screenY = (1.0f - ndcY) * 0.5f * WinCore::g_dwClientHeight;
				preview.pNameTag->SetPosition(Vector2{ screenX, screenY });
			}
			else {
				preview.pNameTag->SetPosition(Vector2{ -10000.f, -10000.f });
			}
		}
	}

	// 서버가 전원 레디 확인 후 보낸 게임 시작 신호 수신 → 전원 동시 게임 진입
	if (NETWORK->ConsumeGameStart()) {
		m_bProceed = true;
	}

	if (m_bProceed) {
		m_bProceed = false;
		//SCENE->ChangeScene<GameScene>();
		SCENE->ChangeSceneAsync<GameScene>();
		return;
	}
}
