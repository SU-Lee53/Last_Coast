#include "pch.h"
#include "SceneManager.h"
#include "LogInScene.h"

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

void SceneManager::CleanUp()
{
	for (auto it = m_pSceneStack.rbegin(); it != m_pSceneStack.rend(); ++it) {
		(*it)->OnLeaveScene();
		(*it)->CleanUp();
	}
	m_pSceneStack.clear();

	{
		std::lock_guard lock{ m_mtxAsyncScene };
		if (m_pAsyncNextScene) {
			m_pAsyncNextScene->CleanUp();
			m_pAsyncNextScene.reset();
		}
	}

	m_AsyncSceneTicket = {};
	m_un64AsyncResourceFence = 0;
	m_un64AsyncTextureFence = 0;
	m_bAsyncSceneReady = false;
	m_bSceneChanged = false;
	m_eAsyncSceneState = ASYNC_SCENE_STATE::IDLE;
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
	TryFinishAsyncSceneChange();

	if (m_bSceneChanged) {
		return;
	}

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

void SceneManager::ResetAsyncSceneChange()
{
	std::lock_guard lock{ m_mtxAsyncScene };

	m_pAsyncNextScene.reset();
	m_AsyncSceneTicket = {};

	m_un64AsyncResourceFence = 0;
	m_un64AsyncTextureFence = 0;
	m_bAsyncSceneReady = false;

	m_eAsyncSceneState = ASYNC_SCENE_STATE::IDLE;
}

void SceneManager::TryFinishAsyncSceneChange()
{
	if (m_eAsyncSceneState == ASYNC_SCENE_STATE::IDLE) {
		return;
	}

	// 1. Check CPU Scene loading work is completed
	if (m_eAsyncSceneState == ASYNC_SCENE_STATE::BUILDING) {
		if (!LOADING->IsComplete(m_AsyncSceneTicket)) {
			return;
		}

		if (LOADING->HasFailed(m_AsyncSceneTicket)) {
			m_eAsyncSceneState = ASYNC_SCENE_STATE::FAILED;
		}
		else {
			m_eAsyncSceneState = ASYNC_SCENE_STATE::WAITING_FOR_GPU;
		}
	}

	// 2. Return to prev scene when loading failed
	if (m_eAsyncSceneState == ASYNC_SCENE_STATE::FAILED) {
		const std::string strError = "Async scene loading failed: " + LOADING->GetLastError() + "\n";

		::OutputDebugStringA(strError.c_str());

		if (m_pSceneStack.size() > 1) {
			PopScene();
		}

		ResetAsyncSceneChange();
		m_bSceneChanged = true;
		return;
	}

	// 3. Check GPU Copy work committed by new scene
	if (m_eAsyncSceneState == ASYNC_SCENE_STATE::WAITING_FOR_GPU) {
		uint64 un64ResourceFence = 0;
		uint64 un64TextureFence = 0;

		{
			std::lock_guard lock{ m_mtxAsyncScene };

			if (!m_bAsyncSceneReady) {
				return;
			}

			un64ResourceFence = m_un64AsyncResourceFence;
			un64TextureFence = m_un64AsyncTextureFence;
		}

		RESOURCE->PollCopyComplete();
		TEXTURE->PollCopyComplete();

		if (!RESOURCE->IsFenceComplete(un64ResourceFence) || !TEXTURE->IsFenceComplete(un64TextureFence)) {
			return;
		}

		m_eAsyncSceneState = ASYNC_SCENE_STATE::COMMITTING;
	}

	if (m_eAsyncSceneState != ASYNC_SCENE_STATE::COMMITTING) {
		return;
	}

	// 4. Move ownership of Scene ptr created by worker thread
	std::unique_ptr<Scene> pNextScene;

	{
		std::lock_guard lock{ m_mtxAsyncScene };
		pNextScene = std::move(m_pAsyncNextScene);
	}

	if (!pNextScene) {
		m_eAsyncSceneState = ASYNC_SCENE_STATE::FAILED;
		return;
	}

	// GPU queues can reference previous scene
	RENDER->WaitForGPUComplete();
	RESOURCE->WaitForCopyComplete();
	TEXTURE->WaitForCopyComplete();

	if (m_pSceneStack.size() >= 2) {
		m_pSceneStack[m_pSceneStack.size() - 2]->OnLeaveScene();
	}

	if (!m_pSceneStack.empty()) {
		m_pSceneStack.back()->OnLeaveScene();
	}

	m_pSceneStack.clear();
	m_pSceneStack.push_back(std::move(pNextScene));

	auto& pCurrentScene = m_pSceneStack.back();
	pCurrentScene->OnEnterScene();
	pCurrentScene->PostInitialize();

	ResetAsyncSceneChange();
	m_bSceneChanged = true;
}
