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
	m_pSceneStack.push_back(std::make_unique<LobbyScene>());
	m_pSceneStack.back()->OnEnterScene();
	m_pSceneStack.back()->BuildLights();
	m_pSceneStack.back()->BuildObjects();
	m_pSceneStack.back()->PostInitialize();

	//RESOURCE->WaitForCopyComplete();
	//TEXTURE->WaitForCopyComplete();
}

void SceneManager::ProcessInput() 
{
	m_pSceneStack.back()->PreProcessInput();
	m_pSceneStack.back()->ProcessInput();
	m_pSceneStack.back()->PostProcessInput();
}

void SceneManager::Update()
{
	m_pSceneStack.back()->PreUpdate();
	m_pSceneStack.back()->Update();
	if (m_bSceneChanged) {
		return;
	}
	m_pSceneStack.back()->FixedUpdate();
	m_pSceneStack.back()->PostUpdate();
}

void SceneManager::PrepareRender()
{
	if (m_bSceneChanged) {
		m_bSceneChanged = false;
		return;
	}
	m_pSceneStack.back()->PrepareRender();
}

void SceneManager::ShowDebugOptions()
{
	ImGui::Begin("Scene");
	m_pSceneStack.back()->ShowDebugOptions();
	ImGui::End();
}
