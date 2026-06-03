#include "pch.h"
#include "SceneManager.h"
#include "AnimationTestScene.h"
#include "NetworkTestScene.h"
#include "TerrainTestScene.h"
#include "MapTestScene.h"
#include "TestScene.h"
#include "GameScene.h"
#include "LogInScene.h"

void SceneManager::Initialize()
{
	m_upCurrentScene = std::make_unique<LogInScene>();
	m_upCurrentScene->BuildLights();
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
	if (m_bSceneChanged) {
		return;
	}
	m_upCurrentScene->FixedUpdate();
	m_upCurrentScene->PostUpdate();
}

void SceneManager::PrepareRender()
{
	if (m_bSceneChanged) {
		m_bSceneChanged = false;
		return;
	}
	m_upCurrentScene->PrepareRender();
}

void SceneManager::ShowDebugOptions()
{
	ImGui::Begin("Scene");
	m_upCurrentScene->ShowDebugOptions();
	ImGui::End();
}
