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
					TryRegister();
				}

				if (NETWORK->m_nRegisterState == 1) {
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "Register Successful! Please log in.");
				} else if (NETWORK->m_nRegisterState == -1) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Register Failed! ID may already exist.");
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Log in")) {
				ImGui::InputText("ID", &m_strIDInput);
				ImGui::InputText("Password", &m_strPasswordInput);
				if (ImGui::Button("Log in")) {
					TryLogIn();
				}

				if (NETWORK->m_nLoginState == 1) {
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "Log in Successful!");
					if (ImGui::Button("Change To Game Scene")) {
						SCENE->ChangeScene<GameScene>();
					}
				} else if (NETWORK->m_nLoginState == -1) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Log in Failed! Invalid ID or Password.");
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
	NETWORK->SendLogin(m_strIDInput, m_strPasswordInput);
	return false;
}

bool LogInScene::TryRegister()
{
	NETWORK->SendRegister(m_strIDInput, m_strPasswordInput);
	return false;
}
