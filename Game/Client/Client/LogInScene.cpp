#include "pch.h"
#include "LogInScene.h"
#include "DebugPlayer.h"
#include "GameScene.h"

void LogInScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();
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


	if (NETWORK->IsConnected()) {
		ImGui::Begin("Log In");

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

				ImGui::EndTabItem();
			}

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

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::SeparatorText("Test");
		ImGui::Text("ID In : %s", m_strIDInput.c_str());
		ImGui::Text("PW In : %s", m_strPasswordInput.c_str());


		ImGui::End();
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
