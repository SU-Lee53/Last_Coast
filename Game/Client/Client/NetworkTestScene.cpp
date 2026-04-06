#include "pch.h"
#include "NetworkTestScene.h"
#include "DebugPlayer.h"

#include "TerrainTestScene.h"

void NetworkTestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();
	//std::shared_ptr<TexturedSprite> pTextureSprite = std::make_shared<TexturedSprite>("Opening", 0.f, 0.f, 1.0f, 1.0f);
	//m_pSprites.push_back(pTextureSprite);

	Scene::InitializeObjects();
}

void NetworkTestScene::OnEnterScene()
{
}

void NetworkTestScene::OnLeaveScene()
{
}

void NetworkTestScene::ProcessInput()
{
}

void NetworkTestScene::Update()
{
	//NETWORK->ConnectToServer();

	//if (ImGui::Button("Change To Scene")) {
	//	SCENE->ChangeScene<TerrainTestScene>();
	//}

	if (NETWORK->IsConnected()) {
		ImGui::Begin("Change Scene");
		{
			if (ImGui::Button("Change To Scene")) {
				SCENE->ChangeScene<TerrainTestScene>();
			}
		}
		ImGui::End();
	}
}
