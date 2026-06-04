#include "pch.h"
#include "MenuScene.h"
#include "DebugPlayer.h"
#include "GameScene.h"
#include "TextBox.h"
#include "MenuScene.h"

void MenuScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	float fWidth = static_cast<float>(WinCore::g_dwClientWidth);
	float fHeight = static_cast<float>(WinCore::g_dwClientHeight);

	// Setup UI
	m_pUIBoard = std::make_unique<UIBoard>();

	// Title
	{
		std::shared_ptr<TextBox> pTitle = std::make_shared<TextBox>(L"Malgun Gothic");
		pTitle->SetText(L"LAST COAST");
		pTitle->SetLayer(0);
		pTitle->SetAnchor(Vector2{ 0.5, 0.0 });
		pTitle->SetPivot(Vector2{ 0.5,0.0 });
		pTitle->SetPosition(Vector2{ 0,200 });
		pTitle->SetTextHeight(120);
		m_pUIBoard->InsertUI(pTitle);
	}

	// Buttons
	{
		std::shared_ptr<TextButton> pPlayButton = std::make_shared<TextButton>(L"Malgun Gothic");
		pPlayButton->SetText(L"Play");
		pPlayButton->SetLayer(0);
		pPlayButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pPlayButton->SetPivot(Vector2{ 0.5,0.0 });
		pPlayButton->SetPosition(Vector2{ 0,400 });
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

		std::shared_ptr<TextButton> pQuitButton = std::make_shared<TextButton>(L"Malgun Gothic");
		pQuitButton->SetText(L"Quit");
		pQuitButton->SetLayer(0);
		pQuitButton->SetAnchor(Vector2{ 0.5, 0.0 });
		pQuitButton->SetPivot(Vector2{ 0.5,0.0 });
		pQuitButton->SetPosition(Vector2{ 0,500 });
		pQuitButton->SetTextHeight(70);


		pQuitButton->SetBeginHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3(1.0, 0.0, 0.0));
			}
		);

		pQuitButton->SetEndHoverCallback(
			[](IUIComponent* pComp) {
				auto pButton = static_cast<TextBox*>(pComp);
				pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
			}
		);

		pQuitButton->SetButtonCallback(
			[](IUIComponent* pComp) {
				PostQuitMessage(0);
			}
		);


		m_pUIBoard->InsertUI(pQuitButton);
	}
}

void MenuScene::OnEnterScene()
{
}

void MenuScene::OnLeaveScene()
{
}

void MenuScene::ProcessInput()
{
}

void MenuScene::Update()
{
	if (m_bProceed) {
		// Proceed to LobbyScene
		SCENE->ChangeScene<GameScene>();	// Test
		return;
	}

	if (INPUT->GetButtonDown(VK_BACK)) {
		SCENE->PopScene();
	}
}
