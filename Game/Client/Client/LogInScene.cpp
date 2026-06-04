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

		pRegisterButton->SetButtonCallback(
			[&](IUIComponent* pComp) {
				m_strIDInput = ::WStringToString(m_pIDInputBox->GetCommittedText());
				m_strPasswordInput = ::WStringToString(m_pPWInputBox->GetCommittedText());
				m_bLastRegisterTry = TryRegister();
			}
		);

		m_pUIBoard->InsertUI(pRegisterButton);
		
		std::shared_ptr<TextButton> pLogInButton = std::make_shared<TextButton>(L"Malgun Gothic");
		pLogInButton->SetText(L"Log In");
		pLogInButton->SetLayer(1);
		pLogInButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
		pLogInButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pLogInButton->SetPivot(Vector2{ 0.0,0.0 });	// pivot to left
		pLogInButton->SetPosition(Vector2{ 50,550 });
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
		m_pResultText = std::make_shared<TextBox>(L"Malgun Gothic");
		m_pResultText->SetText(L"");
		m_pResultText->SetLayer(1);
		m_pResultText->SetAnchor(Vector2{ 0.5, 0.0 });
		m_pResultText->SetPivot(Vector2{ 0.5,0.0 });	// pivot to left
		m_pResultText->SetPosition(Vector2{ 0,650 });
		m_pResultText->SetTextHeight(30);
		m_pUIBoard->InsertUI(m_pResultText);
	}

	// Enter game button
	{
		std::shared_ptr<TextButton> pPlayButton = std::make_shared<TextButton>(L"Malgun Gothic");
		pPlayButton->SetText(L"Enter");
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
	m_pResultText->SetText(L"Try Login");
	// 로그인 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.

	return true;
}

bool LogInScene::TryRegister()
{
	m_pResultText->SetText(L"Try Register");
	// 회원가입 시도
	// 성공하면 TRUE, 실패하면 FALSE 를 리턴
	// ID, 비번은 m_strIDInput, m_strPasswordInput 에 보관됩니다.

	return true;
}
