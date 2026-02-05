#include "pch.h"
#include "SceneManager.h"
#include "AnimationTestScene.h"
#include "NetworkTestScene.h"
#include "TestScene.h"

void SceneManager::Initialize()
{
	//m_upCurrentScene = std::make_unique<AnimationTestScene>();
	//m_upCurrentScene->BuildObjects();
	
	m_upCurrentScene = std::make_unique<NetworkTestScene>();
	m_upCurrentScene->BuildObjects();

	m_upCurrentScene->PostInitialize();

	//RESOURCE->WaitForCopyComplete();
	//TEXTURE->WaitForCopyComplete();
}

void SceneManager::ProcessInput() 
{
	m_upCurrentScene->PreProcessInput();
	m_upCurrentScene->ProcessInput();
	m_upCurrentScene->PostProcessInput();
}

void SceneManager::Update()
{
	m_upCurrentScene->PreUpdate();
	m_upCurrentScene->Update();
	m_upCurrentScene->PostUpdate();
}

void SceneManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	m_upCurrentScene->Render(pd3dCommansList);
}
