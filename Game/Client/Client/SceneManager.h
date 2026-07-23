#pragma once
#include "Scene.h"
#include "LoadingManager.h"
#include "LoadingScene.h"
#include <stack>

enum class ASYNC_SCENE_STATE : uint8 {
	IDLE,
	BUILDING,
	WAITING_FOR_GPU,
	COMMITTING,
	FAILED
};


class SceneManager {

	DECLARE_SINGLE(SceneManager)

public:
	void Initialize();
	void CleanUp() {}

public:
	const std::unique_ptr<Scene>& GetCurrentScene() const { return m_pSceneStack.back(); }

	template<typename T> requires std::derived_from<T, Scene>
	void ChangeScene();

	template<typename T> requires std::derived_from<T, Scene>
	void PushScene();
	void PopScene() {
		m_pSceneStack.back()->OnLeaveScene();
		m_pSceneStack.pop_back();
	}

	template<typename T> requires std::derived_from<T, Scene>
	void ChangeSceneAsync();

	bool IsInAsyncSceneChanging() const { return m_eAsyncSceneState != ASYNC_SCENE_STATE::IDLE; }

public:
	void ProcessInput();
	void Update();
	void PrepareRender();

public:
	void ShowDebugOptions();

private:
	void ResetAsyncSceneChange();
	void TryFinishAsyncSceneChange();

private:
	std::vector<std::unique_ptr<Scene>> m_pSceneStack;
	bool m_bSceneChanged = false;

	// Async scene changing
	mutable std::mutex m_mtxAsyncScene;

	ASYNC_SCENE_STATE m_eAsyncSceneState = ASYNC_SCENE_STATE::IDLE;

	LoadingTicket m_AsyncSceneTicket{};	// Loading ticket for new scene

	std::unique_ptr<Scene> m_pAsyncNextScene;

	uint64 m_un64AsyncResourceFence = 0;
	uint64 m_un64AsyncTextureFence = 0;

	bool m_bAsyncSceneReady = false;

};

template<typename T> requires std::derived_from<T, Scene>
inline void SceneManager::ChangeScene()
{
	m_pSceneStack.back()->OnLeaveScene();
	RENDER->WaitForGPUComplete();

	m_pSceneStack.clear();
	m_pSceneStack.push_back(std::make_unique<T>());
	m_pSceneStack.back()->OnEnterScene();
	m_pSceneStack.back()->BuildLights();
	m_pSceneStack.back()->BuildObjects();
	m_pSceneStack.back()->PostInitialize();

	m_bSceneChanged = true;
}

template<typename T> requires std::derived_from<T, Scene>
inline void SceneManager::PushScene()
{
	m_pSceneStack.push_back(std::make_unique<T>());
	auto& pNewScene = m_pSceneStack.back();

	pNewScene->OnEnterScene();
	pNewScene->BuildLights();
	pNewScene->BuildObjects();
	pNewScene->PostInitialize();
}

template<typename T> requires std::derived_from<T, Scene>
inline void SceneManager::ChangeSceneAsync()
{
	if (m_eAsyncSceneState != ASYNC_SCENE_STATE::IDLE) {
		return;
	}

	{
		std::lock_guard lock{ m_mtxAsyncScene };
		m_pAsyncNextScene.reset();
		m_un64AsyncResourceFence = 0;
		m_un64AsyncTextureFence = 0;
		m_bAsyncSceneReady = false;
	}

	m_AsyncSceneTicket = {};
	m_eAsyncSceneState = ASYNC_SCENE_STATE::BUILDING;

	PushScene<LoadingScene>();

	RESOURCE->WaitForCopyComplete();
	TEXTURE->WaitForCopyComplete();

	m_bSceneChanged = true;

	m_AsyncSceneTicket = LOADING->Enqueue([this]() {
		auto pNextScene = std::make_unique<T>();
		pNextScene->BuildLights();
		pNextScene->BuildObjects();

		const uint64 un64ResourceFenceValue = RESOURCE->GetLastSubmittedFenceValue();
		const uint64 un64TextureFenceValue = TEXTURE->GetLastSubmittedFenceValue();

		{
			std::lock_guard lock{ m_mtxAsyncScene };
			m_pAsyncNextScene = std::move(pNextScene);
			m_un64AsyncResourceFence = un64ResourceFenceValue;
			m_un64AsyncTextureFence = un64TextureFenceValue;
			m_bAsyncSceneReady = true;
		}
	});
}
