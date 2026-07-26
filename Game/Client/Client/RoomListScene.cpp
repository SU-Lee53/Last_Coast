#include "pch.h"
#include "RoomListScene.h"
#include "DebugPlayer.h"
#include "LobbyScene.h"
#include "LogInScene.h"
#include "TextBox.h"
#include "NetworkManager.h"
#include "Utility.h"

void RoomListScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();
	m_pUIBoard = std::make_unique<UIBoard>();
	// Title
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Noto Sans KR");
		pTitle->SetText(L"LAST COAST - ROOM LIST");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5f, 0.0f });
		pTitle->SetPivot(Vector2{ 0.5f, 0.0f });
		pTitle->SetPosition(Vector2{ 0.f, 100.f });
		pTitle->SetTextHeight(90.f);
		m_pUIBoard->InsertUI(pTitle);
	}
	// Create Room Input & Button
	{
		std::shared_ptr<TextBox> pLabel = std::make_shared<TextBox>(L"Noto Sans KR");
		pLabel->SetText(L"Room Name: ");
		pLabel->SetLayer(1);
		pLabel->SetAnchor(Vector2{ 0.5f, 0.0f });
		pLabel->SetPivot(Vector2{ 1.0f, 0.0f });
		pLabel->SetPosition(Vector2{ -120.f, 230.f });
		pLabel->SetTextHeight(40.f);
		m_pUIBoard->InsertUI(pLabel);
		m_pRoomNameInputBox = std::make_shared<InputTextBox>(L"Noto Sans KR");
		m_pRoomNameInputBox->SetPlaceholder(L"Enter Room Name");
		m_pRoomNameInputBox->SetLayer(1);
		m_pRoomNameInputBox->SetAnchor(Vector2{ 0.5f, 0.0f });
		m_pRoomNameInputBox->SetPivot(Vector2{ 0.0f, 0.0f });
		m_pRoomNameInputBox->SetPosition(Vector2{ -100.f, 230.f });
		m_pRoomNameInputBox->SetTextHeight(40.f);
		m_pUIBoard->InsertUI(m_pRoomNameInputBox);
		std::shared_ptr<TextButton> pCreateBtn = std::make_shared<TextButton>(L"Noto Sans KR");
		pCreateBtn->SetText(L"Create Room");
		pCreateBtn->SetLayer(1);
		pCreateBtn->SetAnchor(Vector2{ 0.5f, 0.0f });
		pCreateBtn->SetPivot(Vector2{ 0.0f, 0.0f });
		pCreateBtn->SetPosition(Vector2{ 180.f, 230.f });
		pCreateBtn->SetTextHeight(40.f);
		pCreateBtn->SetButtonCallback([this](IUIComponent*) {
			std::string name = ::WStringToString(m_pRoomNameInputBox->GetCommittedText());
			if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
				NETWORK->SendCreateRoom(name);
				m_pStatusText->SetText(L"Creating room...");
			}
			});
		m_pUIBoard->InsertUI(pCreateBtn);
	}
	// Refresh & Log Out Buttons
	{
		std::shared_ptr<TextButton> pRefreshBtn = std::make_shared<TextButton>(L"Noto Sans KR");
		pRefreshBtn->SetText(L"Refresh");
		pRefreshBtn->SetLayer(1);
		pRefreshBtn->SetAnchor(Vector2{ 0.5f, 0.0f });
		pRefreshBtn->SetPivot(Vector2{ 1.0f, 0.0f });
		pRefreshBtn->SetPosition(Vector2{ -200.f, 300.f });
		pRefreshBtn->SetTextHeight(40.f);
		pRefreshBtn->SetButtonCallback([this](IUIComponent*) {
			if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
				NETWORK->SendRoomListReq();
				m_pStatusText->SetText(L"Refreshing room list...");
			}
			});
		m_pUIBoard->InsertUI(pRefreshBtn);
		std::shared_ptr<TextButton> pLogOutBtn = std::make_shared<TextButton>(L"Noto Sans KR");
		pLogOutBtn->SetText(L"Log Out");
		pLogOutBtn->SetLayer(1);
		pLogOutBtn->SetAnchor(Vector2{ 0.5f, 0.0f });
		pLogOutBtn->SetPivot(Vector2{ 0.0f, 0.0f });
		pLogOutBtn->SetPosition(Vector2{ 200.f, 300.f });
		pLogOutBtn->SetTextHeight(40.f);
		pLogOutBtn->SetButtonCallback([this](IUIComponent*) {
			NETWORK->Disconnect();
			SCENE->ChangeScene<LogInScene>();
			});
		m_pUIBoard->InsertUI(pLogOutBtn);
	}
	// Status Text
	{
		m_pStatusText = std::make_shared<TextBox>(L"Noto Sans KR");
		m_pStatusText->SetText(L"Searching for rooms...");
		m_pStatusText->SetLayer(1); m_pStatusText->SetAnchor(Vector2{ 0.5f, 0.0f });
		m_pStatusText->SetPivot(Vector2{ 0.5f, 0.0f });
		m_pStatusText->SetPosition(Vector2{ 0.f, 360.f });
		m_pStatusText->SetTextHeight(30.f);
		m_pUIBoard->InsertUI(m_pStatusText);
	}
}

void RoomListScene::OnEnterScene()
{
	NETWORK->m_nCreateRoomState = 0;
	NETWORK->m_nJoinRoomState = 0;
	if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
		NETWORK->SendRoomListReq();
	}
}

void RoomListScene::OnLeaveScene()
{
	for (auto& item : m_RoomUIItems) {
		if (item.pJoinButton) {
			m_pUIBoard->RemoveUI(item.pJoinButton);
		}
	}
	m_RoomUIItems.clear();
}

void RoomListScene::ProcessInput()
{
}

void RoomListScene::Update()
{
	
	if (NETWORK->ConsumeRoomListChanged()) {
		RefreshRoomListUI();
	}
	if (NETWORK->m_nCreateRoomState == 1) {
		NETWORK->m_nCreateRoomState = 0;
		m_pStatusText->SetText(L"Room created!");
		SCENE->ChangeScene<LobbyScene>();
		return;
	}
	else if (NETWORK->m_nCreateRoomState == -1) {
		std::wstring err = ::StringToWString(NETWORK->m_strRoomErrMsg);
		m_pStatusText->SetText(L"Create room failed: " + err);
		NETWORK->m_nCreateRoomState = 0;
	}
	if (NETWORK->m_nJoinRoomState == 1) {
		NETWORK->m_nJoinRoomState = 0;
		m_pStatusText->SetText(L"Joined room!");
		SCENE->ChangeScene<LobbyScene>();
		return;
	}
	else if (NETWORK->m_nJoinRoomState == -1) {
		std::wstring err = ::StringToWString(NETWORK->m_strRoomErrMsg);
		m_pStatusText->SetText(L"Join room failed: " + err);
		NETWORK->m_nJoinRoomState = 0;
	}
}

void RoomListScene::RefreshRoomListUI()
{
	auto rooms = NETWORK->GetRoomList();
	// Remove old buttons
	for (auto& item : m_RoomUIItems) {
		if (item.pJoinButton) {
			m_pUIBoard->RemoveUI(item.pJoinButton);
		}
	}
	m_RoomUIItems.clear();

	float fStartY = 420.f;
	float fRowGap = 50.f;
	if (rooms.empty()) {
		if (m_pStatusText && NETWORK->m_nCreateRoomState == 0 && NETWORK->m_nJoinRoomState == 0) {
			m_pStatusText->SetText(L"No rooms available. Create a new room above!");
		}
		return;
	}
	for (size_t i = 0; i < rooms.size(); ++i) {
		const auto& info = rooms[i];
		std::wstring wstrRoomName = ::StringToWString(info.roomName);
		std::wstring wstrStatus = info.bInGame ? L"[IN GAME]" : L"[WAITING]";
		std::wstring wstrLabel = std::format(L"Room #{}: {} ({}/{}) {}",
			info.roomId + 1, wstrRoomName, info.playerCount, info.maxPlayers, wstrStatus);
		std::shared_ptr<TextButton> pBtn = std::make_shared<TextButton>(L"Noto Sans KR");
		pBtn->SetText(wstrLabel);
		pBtn->SetLayer(1);
		pBtn->SetAnchor(Vector2{ 0.5f, 0.0f });
		pBtn->SetPivot(Vector2{ 0.5f, 0.0f });
		pBtn->SetPosition(Vector2{ 0.f, fStartY + i * fRowGap });
		pBtn->SetTextHeight(36.f);
		int rId = info.roomId;
		bool bJoinable = !info.bInGame && (info.playerCount < info.maxPlayers);
		if (!bJoinable) {
			pBtn->SetColor(Vector3{ 0.4f, 0.4f, 0.4f });
		}
		pBtn->SetButtonCallback([this, rId, bJoinable](IUIComponent*) {
			if (!bJoinable) {
				m_pStatusText->SetText(L"Cannot join this room!");
				return;
			}
			if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
				NETWORK->SendJoinRoom(rId);
				m_pStatusText->SetText(L"Joining room...");
			}
			});
		m_pUIBoard->InsertUI(pBtn);
		m_RoomUIItems.push_back(RoomUIItem{ pBtn, rId });
	}
	if (m_pStatusText && NETWORK->m_nCreateRoomState == 0 && NETWORK->m_nJoinRoomState == 0) {
		m_pStatusText->SetText(std::format(L"Found {} room(s). Click to join!", rooms.size()));
	}
}
