#include "pch.h"
#include "SceneManager.h"
#include "TestScene.h"

void SceneManager::Initialize()
{
	m_upCurrentScene = std::make_unique<TestScene>();
	m_upCurrentScene->BuildObjects();
}

void SceneManager::ProcessInput() 
{
	m_upCurrentScene->ProcessInput();
}

void SceneManager::Update()
{
	m_upCurrentScene->Update();
}

void SceneManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	m_upCurrentScene->Render(pd3dCommansList);
}
