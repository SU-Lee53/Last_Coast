#include "pch.h"
#include "MapTestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "Skybox.h"
#include "GameScene.h"
#include "TextBox.h"
#include "WeaponObject.h"

void MapTestScene::BuildObjects()
{
	m_pUIBoard = std::make_unique<UIBoard>();

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	m_pPlayer = std::make_shared<DebugPlayer>();
	m_pPlayer->Initialize();

	LoadFromFiles("LightTest");
}

void MapTestScene::OnEnterScene()
{
}

void MapTestScene::OnLeaveScene()
{
}

void MapTestScene::ProcessInput()
{
}

void MapTestScene::Update()
{
	if (INPUT->GetButtonDown(VK_F2)) {
		m_bShowConfig = !m_bShowConfig;
	}

	if (m_bShowConfig) {
		ImGui::Begin("Test");
		{
			if (ImGui::BeginTabBar("Config")) {

				if (ImGui::BeginTabItem("Lights")) {
					ImGui::Text("Elapsed TIme : %f", TIME->GetTimeElapsed());
					ImGui::Text("Total TIme : %f", TIME->GetTotalTime());

					float fAmbient = m_v4GlobalAmbient.x;
					ImGui::DragFloat("GlobalAmbient", (float*)&fAmbient, 0.001f, 0.f, 1.f);
					m_v4GlobalAmbient = XMVectorReplicate(fAmbient);
					ImGui::Text("NumLights : %d", m_pLights.size());
					for (uint32 i = 0; i < m_pLights.size(); ++i) {
						if (ImGui::TreeNode(std::format("Index : {}", i).c_str())) {
							m_pLights[i]->ShowControllImGui();
							ImGui::TreePop();
						}
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Skybox")) {
					if (m_pSkybox) {
						m_pSkybox->ShowControllImGui();
					}
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

}
