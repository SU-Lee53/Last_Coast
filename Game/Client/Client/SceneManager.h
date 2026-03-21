#pragma once
#include "Scene.h"	// Scene.h 포함

class SceneManager {

	DECLARE_SINGLE(SceneManager)

public:
	void Initialize();
	void CleanUp() {}

public:
	const std::unique_ptr<Scene>& GetCurrentScene() const { return m_upCurrentScene; }

	template<typename T> requires std::derived_from<T, Scene>
	void ChangeScene();

public:
	void ProcessInput();
	void Update();
	void PrepareRender();
	
private:
	std::unique_ptr<Scene> m_upCurrentScene;

	bool m_bSceneChanged = false;


};

template<typename T> requires std::derived_from<T, Scene>
inline void SceneManager::ChangeScene()
{
	m_upCurrentScene->OnLeaveScene();
	RENDER->WaitForGPUComplete();

	m_upCurrentScene.reset(new T());
	m_upCurrentScene->OnEnterScene();
	m_upCurrentScene->BuildLights();
	m_upCurrentScene->BuildObjects();
	m_upCurrentScene->PostInitialize();

	Update();

	m_bSceneChanged = true;
}
