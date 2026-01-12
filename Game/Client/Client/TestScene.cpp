#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"

void TestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	LoadFromFiles("TEST");

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


}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	Scene::RenderObjects(pd3dCommansList);
}
