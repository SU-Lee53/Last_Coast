#include "pch.h"
#include "LogInScene.h"
#include "DebugPlayer.h"
#include "GameScene.h"
#include "TextBox.h"
#include "MenuScene.h"
#include "Sprite.h"
#include "RoomListScene.h"

void LogInScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	float fWidth = static_cast<float>(WinCore::g_dwClientWidth);
	float fHeight = static_cast<float>(WinCore::g_dwClientHeight);

	// Setup UI
	m_pUIBoard = std::make_unique<UIBoard>();

	// IP input
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Noto Sans KR");
		pTitle->SetText(L"Server IP : ");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 1.0,0.0 });
		pTitle->SetPosition(Vector2{ -100,400 });
		pTitle->SetTextHeight(50);
		m_pUIBoard->InsertUI(pTitle);

		m_pServerIPInputBox = std::make_shared<InputTextBox>(L"Noto Sans KR");
		m_pServerIPInputBox->SetCommittedText(L"127.0.0.1");
		m_pServerIPInputBox->SetLayer(1);
		m_pServerIPInputBox->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pServerIPInputBox->SetPivot(Vector2{ 0.0,0.0 });
		m_pServerIPInputBox->SetPosition(Vector2{ -50,400 });
		m_pServerIPInputBox->SetTextHeight(50);
		m_pUIBoard->InsertUI(m_pServerIPInputBox);

		std::shared_ptr<TextButton> pConnectButton = std::make_shared<TextButton>(L"Noto Sans KR");
		pConnectButton->SetText(L"Connect");
		pConnectButton->SetLayer(1);
		pConnectButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pConnectButton->SetPivot(Vector2{ 0.0,0.0 });
		pConnectButton->SetPosition(Vector2{ 180,400 });
		pConnectButton->SetTextHeight(40);
		pConnectButton->SetButtonCallback(
			[this](IUIComponent*) {
				m_strServerIPInput = ::WStringToString(m_pServerIPInputBox->GetCommittedText());
				m_bConnectionResultHandled = false;
				NETWORK->RequestConnect(m_strServerIPInput);
			}
		);
		m_pUIBoard->InsertUI(pConnectButton);
	}

	// Title
	{
		std::shared_ptr<ImageBox> pTitle = std::make_shared<ImageBox>("../Resources/Textures/logo.png");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 0.5,0.0 });
		pTitle->SetPosition(Vector2{ 0,200 });
		pTitle->SetSize(Vector2{ 600,200 });
		m_pUIBoard->InsertUI(pTitle);
	}

	// ID input
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Noto Sans KR");
		pTitle->SetText(L"ID : ");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 1.0,0.0 });
		pTitle->SetPosition(Vector2{ -100,500 });
		pTitle->SetTextHeight(50);
		m_pUIBoard->InsertUI(pTitle);

		m_pIDInputBox = std::make_shared<InputTextBox>(L"Noto Sans KR");
		m_pIDInputBox->SetPlaceholder(L"Example ID");
		m_pIDInputBox->SetLayer(1);
		m_pIDInputBox->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pIDInputBox->SetPivot(Vector2{ 0.0,0.0 });
		m_pIDInputBox->SetPosition(Vector2{ -50,500 });
		m_pIDInputBox->SetTextHeight(50);
		m_pUIBoard->InsertUI(m_pIDInputBox);
	}

	// Password input
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Noto Sans KR");
		pTitle->SetText(L"Password : ");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 1.0,0.0 });
		pTitle->SetPosition(Vector2{ -100,550 });
		pTitle->SetTextHeight(50);
		m_pUIBoard->InsertUI(pTitle);

		m_pPWInputBox = std::make_shared<InputTextBox>(L"Noto Sans KR");
		m_pPWInputBox->SetPlaceholder(L"Example PW");
		m_pPWInputBox->SetLayer(1);
		m_pPWInputBox->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pPWInputBox->SetPivot(Vector2{ 0.0,0.0 });
		m_pPWInputBox->SetPosition(Vector2{ -50,550 });
		m_pPWInputBox->SetTextHeight(50);
		m_pPWInputBox->SetPasswordMode(true);
		m_pUIBoard->InsertUI(m_pPWInputBox);
	}

	// Register / Login button
	{
		std::shared_ptr<TextButton> pRegisterButton = std::make_shared<TextButton>(L"Noto Sans KR");
		pRegisterButton->SetText(L"Register");
		pRegisterButton->SetLayer(1);
		pRegisterButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
		pRegisterButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pRegisterButton->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
		pRegisterButton->SetPosition(Vector2{ -50,620 });
		pRegisterButton->SetTextHeight(50);

		pRegisterButton->SetBeginHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3(1.0, 0.0, 0.0));
			}
		);

		pRegisterButton->SetEndHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
			}
		);

		pRegisterButton->SetButtonCallback(
			[&](IUIComponent* pComp) {
				m_strIDInput = ::WStringToString(m_pIDInputBox->GetCommittedText());
				m_strPasswordInput = ::WStringToString(m_pPWInputBox->GetCommittedText());
				m_bLastRegisterTry = TryRegister();
			}
		);

		m_pUIBoard->InsertUI(pRegisterButton);

		std::shared_ptr<TextButton> pLogInButton = std::make_shared<TextButton>(L"Noto Sans KR");
		pLogInButton->SetText(L"Log In");
		pLogInButton->SetLayer(1);
		pLogInButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
		pLogInButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pLogInButton->SetPivot(Vector2{ 0.0,0.0 });	// pivot to left
		pLogInButton->SetPosition(Vector2{ 50,620 });
		pLogInButton->SetTextHeight(50);


		pLogInButton->SetBeginHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3(1.0, 0.0, 0.0));
			}
		);

		pLogInButton->SetEndHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
			}
		);

		pLogInButton->SetButtonCallback(
			[&](IUIComponent* pComp) {
				m_strIDInput = ::WStringToString(m_pIDInputBox->GetCommittedText());
				m_strPasswordInput = ::WStringToString(m_pPWInputBox->GetCommittedText());
				m_bLastLogInTry = TryLogIn();
			}
		);

		m_pUIBoard->InsertUI(pLogInButton);
	}

	// Register / Log In Result
	{
		m_pResultText = std::make_shared<TextBox>(L"Noto Sans KR");
		m_pResultText->SetText(L"");
		m_pResultText->SetLayer(1);
		m_pResultText->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pResultText->SetPivot(Vector2{ 0.5,0.0 });	// pivot to left
		m_pResultText->SetPosition(Vector2{ 0,690 });
		m_pResultText->SetTextHeight(30);
		m_pUIBoard->InsertUI(m_pResultText);
	}

	// Enter game button
	{
		std::shared_ptr<TextButton> pPlayButton = std::make_shared<TextButton>(L"Noto Sans KR");
		pPlayButton->SetText(L"Enter Offline");
		pPlayButton->SetLayer(1);
		pPlayButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pPlayButton->SetPivot(Vector2{ 0.5,0.0 });
		pPlayButton->SetPosition(Vector2{ 0,750 });
		pPlayButton->SetTextHeight(70);

		pPlayButton->SetBeginHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3(1.0, 0.0, 0.0));
			}
		);

		pPlayButton->SetEndHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
			}
		);

		pPlayButton->SetButtonCallback(
			[&](IUIComponent* pComp) {
				m_bProceed = true;
			}
		);

		m_pUIBoard->InsertUI(pPlayButton);
	}

}

void LogInScene::OnEnterScene()
{
}

void LogInScene::OnLeaveScene()
{
	GameContext::PlayerData data;
	data.m_nPlayerID = 0;
	data.m_nPlayerName = ::WStringToString(m_pIDInputBox->GetCommittedText());;
	GCTX->SetPlayerData(data);
}

void LogInScene::ProcessInput()
{
}

void LogInScene::Update()
{
	NETWORK->ConnectToServer();

	if (!m_bConnectionResultHandled) {
		switch (NETWORK->GetConnectState())
		{
		case ConnectState::Connecting:
			m_pResultText->SetText(L"Connecting...");
			break;
		case ConnectState::Connected:
			m_pResultText->SetText(L"Connected!");
			m_bConnectionResultHandled = true;
			break;
		case ConnectState::Failed:
			m_pResultText->SetText(::StringToWString(NETWORK->GetErrorLog()));
			m_bConnectionResultHandled = true;
			break;
		}
	}

	if (NETWORK->m_nLoginState == 1) {
		m_pResultText->SetText(L"Login Success!");
		NETWORK->m_nLoginState = 0;
		m_bProceed = true;
	}
	else if (NETWORK->m_nLoginState == -1) {
		m_pResultText->SetText(L"Login Failed. Please check ID/PW.");
		NETWORK->m_nLoginState = 0;
	}

	if (NETWORK->m_nRegisterState == 1) {
		m_pResultText->SetText(L"Register Success!");
		NETWORK->m_nRegisterState = 0;
	}
	else if (NETWORK->m_nRegisterState == -1) {
		m_pResultText->SetText(L"Register Failed. ID may already exist.");
		NETWORK->m_nRegisterState = 0;
	}

	if (m_bProceed) {
		m_bProceed = false;
		SCENE->ChangeScene<RoomListScene>();
		return;
	}

}

bool LogInScene::TryLogIn()
{
	// 로그인 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.
	if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
		m_pResultText->SetText(L"Try Login");
		NETWORK->SendLogin(m_strIDInput, m_strPasswordInput);
		return true;
	}

	m_pResultText->SetText(L"Connect to server first.");
	return false;
}

bool LogInScene::TryRegister()
{
	// 회원가입 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.
	if (NETWORK->IsConnected() && !NETWORK->IsOffline()) {
		m_pResultText->SetText(L"Try Register");
		NETWORK->SendRegister(m_strIDInput, m_strPasswordInput);
		return true;
	}

	m_pResultText->SetText(L"Connect to server first.");
	return false;
}
