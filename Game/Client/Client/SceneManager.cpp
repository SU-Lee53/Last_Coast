#include "pch.h"
#include "SceneManager.h"
#include "AnimationTestScene.h"
#include "TestScene.h"

void SceneManager::Initialize()
{
	m_upCurrentScene = std::make_unique<AnimationTestScene>();
	m_upCurrentScene->BuildObjects();

	//m_upCurrentScene = std::make_unique<TestScene>();
	//m_upCurrentScene->BuildObjects();

	//RESOURCE->WaitForCopyComplete();
	//TEXTURE->WaitForCopyComplete();
}

void SceneManager::ProcessInput() 
{
	m_upCurrentScene->OnPreProcessInput();
	m_upCurrentScene->ProcessInput();
	m_upCurrentScene->OnPostProcessInput();
}

void SceneManager::Update()
{
	m_upCurrentScene->OnPreUpdate();
	m_upCurrentScene->Update();
	m_upCurrentScene->OnPostUpdate();
}

void SceneManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	m_upCurrentScene->Render(pd3dCommansList);
}
