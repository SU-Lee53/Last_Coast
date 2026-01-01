#pragma once
#include "Player.h"	// Includes GameObject
#include "Sprite.h"	// Includes GameObject
#include "Camera.h"
#include "Light.h"

class Scene {
public:
	~Scene() {}


public:
	virtual void BuildObjects() = 0;
	virtual void BuildLights() {}

public:
	void AddObject(std::shared_ptr<GameObject> pObj) {
		m_pGameObjects.push_back(pObj);
	}

	template<typename... Objs, 
		typename = std::enable_if_t<(std::is_same_v<Objs, std::shared_ptr<GameObject>> && ...)>>
	void AddObjects(Objs... pObjs) {
		(m_pGameObjects.push_back(std::forward<Objs>(pObjs)), ...);
	}

public:
	virtual void OnEnterScene() = 0;
	virtual void OnLeaveScene() = 0;

public:
	virtual void ProcessInput() = 0;
	virtual void Update() = 0;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList) = 0;

	void OnPreProcessInput();
	void OnPostProcessInput();
	void OnPreUpdate();
	void OnPostUpdate();

	virtual void SyncSceneWithServer() {}

public:
	const std::shared_ptr<Player>& GetPlayer() const { return m_pPlayer; }
	const std::shared_ptr<Camera>& GetCamera() const { return m_pPlayer->GetCamera(); }
	std::vector<std::shared_ptr<GameObject>>& GetObjectsInScene() { return m_pGameObjects; }

	CB_LIGHT_DATA MakeLightData();

protected:
	void InitializeObjects();
	void UpdateObjects();
	void RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);


protected:
	std::vector<std::shared_ptr<GameObject>> m_pGameObjects = {};
	std::vector<std::shared_ptr<Sprite>>		m_pSprites;
	std::vector<std::shared_ptr<Light>> m_pLights = {};
	
	std::shared_ptr<Player> m_pPlayer = nullptr;
	//std::shared_ptr<Camera> m_pMainCamera = nullptr;

	Vector4 m_v4GlobalAmbient;
};

