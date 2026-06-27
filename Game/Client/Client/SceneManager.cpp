#include "pch.h"
#include "SceneManager.h"
#include "AnimationTestScene.h"
#include "NetworkTestScene.h"
#include "TerrainTestScene.h"
#include "MapTestScene.h"
#include "TestScene.h"
#include "GameScene.h"
#include "LogInScene.h"
#include "LobbyScene.h"

void SceneManager::Initialize()
{
	m_pSceneStack.push_back(std::make_unique<LogInScene>());
	auto& pCurScene = m_pSceneStack.back();
	pCurScene->OnEnterScene();
	pCurScene->BuildLights();
	pCurScene->BuildObjects();
	pCurScene->PostInitialize();

	//RESOURCE->WaitForCopyComplete();
	//TEXTURE->WaitForCopyComplete();
}

void SceneManager::ProcessInput() 
{
	auto& pCurScene = m_pSceneStack.back();
	pCurScene->PreProcessInput();
	pCurScene->ProcessInput();
	pCurScene->PostProcessInput();
}

void SceneManager::Update()
{
	auto& pCurScene = m_pSceneStack.back();
	pCurScene->PreUpdate();
	pCurScene->Update();
	if (m_bSceneChanged) {
		return;
	}

	// Get new refernce in case of scene changed
	auto& pNewScene = m_pSceneStack.back();
	pNewScene->FixedUpdate();
	pNewScene->PostUpdate();
}

void SceneManager::PrepareRender()
{
	if (m_bSceneChanged) {
		m_bSceneChanged = false;
		return;
	}

	auto& pCurScene = m_pSceneStack.back();
	pCurScene->PrepareRender();
}

void SceneManager::ShowDebugOptions()
{
	auto& pCurScene = m_pSceneStack.back();
	ImGui::Begin("Scene");
	pCurScene->ShowDebugOptions();
	ImGui::End();
}
