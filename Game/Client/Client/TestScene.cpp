#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"

void TestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	std::shared_ptr<GameObject> pGameObject = MODEL->Get("Ch33_nonPBR");
	m_pGameObjects.push_back(pGameObject);

	Scene::InitializeObjects();
}

void TestScene::OnEnterScene()
{
}

void TestScene::OnLeaveScene()
{
}

void TestScene::ProcessInput()
{
}

void TestScene::Update()
{
	ImGui::Begin("Test");
	{
	}
	ImGui::End();


	Scene::UpdateObjects();
}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	Scene::RenderObjects(pd3dCommansList);
}
