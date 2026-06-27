#pragma once
#include "Scene.h"	// Scene.h 포함
#include <stack>

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
		m_pSceneStack.pop_back();
	}

public:
	void ProcessInput();
	void Update();
	void PrepareRender();

public:
	void ShowDebugOptions();

private:
	//std::unique_ptr<Scene> m_upCurrentScene;
	std::vector<std::unique_ptr<Scene>> m_pSceneStack;

	bool m_bSceneChanged = false;


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
	std::unique_ptr<Scene> pNewScene = std::make_unique<T>();
	pNewScene->OnEnterScene();
	pNewScene->BuildLights();
	pNewScene->BuildObjects();
	pNewScene->PostInitialize();
	m_pSceneStack.push_back(std::move(pNewScene));
}
