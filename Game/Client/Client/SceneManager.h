#pragma once
#include "Scene.h"	// Scene.h 포함

class SceneManager {
public:
	SceneManager();
	~SceneManager();

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
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList);

private:
	std::unique_ptr<Scene> m_upCurrentScene;

};

template<typename T> requires std::derived_from<T, Scene>
inline void SceneManager::ChangeScene()
{
	m_upCurrentScene->OnLeaveScene();
	m_upCurrentScene.reset(new T());
	m_upCurrentScene->BuildObjects();
	m_upCurrentScene->OnEnterScene();
}
