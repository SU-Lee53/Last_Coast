#include "pch.h"
#include "LogInScene.h"
#include "DebugPlayer.h"
#include "GameScene.h"
#include "TextBox.h"
#include "MenuScene.h"

void LogInScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	float fWidth = static_cast<float>(WinCore::g_dwClientWidth);
	float fHeight = static_cast<float>(WinCore::g_dwClientHeight);

	// Setup UI
	m_pUIBoard = std::make_unique<UIBoard>();

	// IP input
	// 연결만 ImGui로?
	/*{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Malgun Gothic");
		pTitle->SetText(L"Server IP : ");
		pTitle->SetLayer(0);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 0.5,0.0 });
		pTitle->SetPosition(Vector2{ 0,400 });
		pTitle->SetTextHeight(30);
		m_pUIBoard->InsertUI(pTitle);

		std::shared_ptr<InputTextBox> pInputIP = std::make_shared<InputTextBox>(L"Malgun Gothic");
		pInputIP->SetPlaceholder(L"Input Here");
		pInputIP->SetLayer(0);
		pInputIP->SetAnchor(Vector2{ 0.5, 0.0 });
		pInputIP->SetPivot(Vector2{ 0.5,0.0 });
		pInputIP->SetPosition(Vector2{ 0,450 });
		pInputIP->SetTextHeight(50);
		m_pUIBoard->InsertUI(pInputIP);
	}*/

	// Title
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Malgun Gothic");
		pTitle->SetText(L"LAST COAST");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 0.5,0.0 });
		pTitle->SetPosition(Vector2{ 0,200 });
		pTitle->SetTextHeight(120);
		m_pUIBoard->InsertUI(pTitle);
	}

	// ID input
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Malgun Gothic");
		pTitle->SetText(L"ID : ");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 1.0,0.0 });
		pTitle->SetPosition(Vector2{ -100,400 });
		pTitle->SetTextHeight(50);
		m_pUIBoard->InsertUI(pTitle);

		m_pIDInputBox = std::make_shared<InputTextBox>(L"Malgun Gothic");
		m_pIDInputBox->SetPlaceholder(L"Example ID");
		m_pIDInputBox->SetLayer(1);
		m_pIDInputBox->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pIDInputBox->SetPivot(Vector2{ 0.0,0.0 });
		m_pIDInputBox->SetPosition(Vector2{ -50,400 });
		m_pIDInputBox->SetTextHeight(50);
		m_pUIBoard->InsertUI(m_pIDInputBox);
	}

	// Password input
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Malgun Gothic");
		pTitle->SetText(L"Password : ");
		pTitle->SetLayer(1);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 1.0,0.0 });
		pTitle->SetPosition(Vector2{ -100,450 });
		pTitle->SetTextHeight(50);
		m_pUIBoard->InsertUI(pTitle);

		m_pPWInputBox = std::make_shared<InputTextBox>(L"Malgun Gothic");
		m_pPWInputBox->SetPlaceholder(L"Example PW");
		m_pPWInputBox->SetLayer(1);
		m_pPWInputBox->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pPWInputBox->SetPivot(Vector2{ 0.0,0.0 });
		m_pPWInputBox->SetPosition(Vector2{ -50,450 });
		m_pPWInputBox->SetTextHeight(50);
		m_pPWInputBox->SetPasswordMode(true);
		m_pUIBoard->InsertUI(m_pPWInputBox);
	}

	// Register / Login button
	{
		std::shared_ptr<TextButton> pRegisterButton = std::make_shared<TextButton>(L"Malgun Gothic");
		pRegisterButton->SetText(L"Register");
		pRegisterButton->SetLayer(1);
		pRegisterButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
		pRegisterButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pRegisterButton->SetPivot(Vector2{ 1.0,0.0 });	// Pivot to right
		pRegisterButton->SetPosition(Vector2{ -50,550 });
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

		if (ImGui::BeginTabBar("Log in")) {
			if (ImGui::BeginTabItem("Register")) {
				ImGui::InputText("ID", &m_strIDInput);
				ImGui::InputText("Password", &m_strPasswordInput);
				if (ImGui::Button("Register")) {
					m_bLastRegisterTry = TryRegister();
				}

				if (m_bLastRegisterTry) {
					ImGui::Text("Register Successful, Try log in");
				}

		pLogInButton->SetEndHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
			}
		);

			if (ImGui::BeginTabItem("Log in")) {
				ImGui::InputText("ID", &m_strIDInput);
				ImGui::InputText("Password", &m_strPasswordInput);
				if (ImGui::Button("Log in")) {
					m_bLastLogInTry = TryLogIn();
				}

				if (m_bLastLogInTry) {
					ImGui::Text("Log in Successful");
					if (ImGui::Button("Change To Scene")) {
						SCENE->ChangeScene<GameScene>();
					}
				}

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

	if (m_bProceed) {
		m_bProceed = false;
		SCENE->PushScene<MenuScene>();
		return;
	}

}

bool LogInScene::TryLogIn()
{
	// 로그인 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.


	return true;
}

bool LogInScene::TryRegister()
{
	// 회원가입 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.

	return true;
}
