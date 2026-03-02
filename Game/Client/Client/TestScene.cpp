#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"

void TestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	LoadFromFiles("Light");

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
