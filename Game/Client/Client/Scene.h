#pragma once
#include "World.h"
#include "GameObject.h"
#include "ToneMappingVolume.h"
#include "PostProcessingVolume.h"

#include "StaticObject.h"
#include "Zombie.h"
#include "WeaponObject.h"

#include "Player.h"	// Includes GameObject
#include "Camera.h"
#include "Light.h"
#include "UIBoard.h"

class TerrainComponent;
class TerrainObject;
class Skybox;
//class Sprite;

using CollisionPair = std::pair<std::shared_ptr<IGameObject>, std::shared_ptr<IGameObject>>;

class Scene {
	friend class SceneManager;

	using WorldType = World<NetworkRemoteThirdPersonPlayer, StaticObject, WeaponObject, Zombie>;

public:
	virtual void BuildObjects() = 0;
	virtual void BuildLights();

public:
	template<typename T> requires std::derived_from<T, IGameObject>
	void AddObject(std::shared_ptr<T> pObj);

	template<typename... Objs, 
		typename = std::enable_if_t<(std::is_same_v<Objs, std::shared_ptr<IGameObject>> && ...)>>
	void AddObjects(Objs... pObjs) {
		(m_pGameObjects.push_back(std::forward<Objs>(pObjs)), ...);
	}

	HRESULT LoadFromFiles(const std::string& strFileName);


public:
	virtual void ProcessInput() = 0;
	virtual void Update() = 0;
	void CleanUp();

	virtual void OnEnterScene() = 0;
	virtual void OnLeaveScene() = 0;

	void PostInitialize();
	void PreProcessInput();
	void PostProcessInput();
	void PreUpdate();
	void FixedUpdate();
	void PostUpdate();
	void PrepareRender();

	void GenerateSceneBound();
	virtual void SyncSceneWithServer() {}

	void CheckCollision();
	void RemoveInvalidCollisionSet(const SpatialQueryResult& playerBroadPhaseResult);

public:
	//const std::vector<std::shared_ptr<IGameObject>>& GetStaticObjectsInScene() const { return m_pStaticObjects; }
	//const std::vector<std::shared_ptr<IGameObject>>& GetDynamicObjectsInScene() const { return m_pDynamicObjects; }

	const WorldType& GetWorld() const { return m_World; }
	const std::shared_ptr<IPlayer>& GetPlayer() const { return m_pPlayer; }
	const std::shared_ptr<TerrainObject>& GetTerrain() const { return m_pTerrain; }
	const std::shared_ptr<Skybox>& GetSkybox() const { return m_pSkybox; }
	const std::shared_ptr<Camera>& GetCamera() const { return m_pPlayer->GetCamera(); }
	const std::vector<std::shared_ptr<Light>>& GetLightsInScene() const { return m_pLights; }
	const Vector4& GetGlobalAmbient() const { return m_v4GlobalAmbient; }
	const std::unique_ptr<UIBoard>& GetUIBoard() const { return m_pUIBoard; }

	const ToneMappingVolume& GetToneMappingVolume() const { return m_ToneMappingVolume; }
	const PostProcessingVolume& GetPostProcessingVolume() const { return m_PostProcessingVolume; }

	std::vector<LightData> MakeLightData() const;

	TerrainHit QueryTerrainHit(const Vector3& v3WorldPos);

protected:
	void RemoveCollisionPairsOf(IGameObject* pDeadObject);

private:
	void InitializeObjects();

private:
	void ShowDebugOptions();


protected:
	WorldType m_World;
	ToneMappingVolume m_ToneMappingVolume{};
	PostProcessingVolume m_PostProcessingVolume{};


	std::vector<std::shared_ptr<Light>>			m_pLights = {};
	
	std::shared_ptr<IPlayer>					m_pPlayer = nullptr;
	std::shared_ptr<TerrainObject>				m_pTerrain = nullptr;
	std::shared_ptr<Skybox>						m_pSkybox = nullptr;

	BoundingBox m_xmSceneBound{};
	std::unique_ptr<UIBoard> m_pUIBoard{};
	std::unordered_set<CollisionResult> m_pCollisionPairs;
	Vector4 m_v4GlobalAmbient;

private:
	bool m_bSpatialRuntimeRegistrationEnabled = false;

public:
	constexpr static float g_fWorldMinX = -500_m;
	constexpr static float g_fWorldMaxX = +500_m;

	constexpr static float g_fWorldMinY = -500_m;
	constexpr static float g_fWorldMaxY = +500_m;

	constexpr static float g_fWorldMinZ = -500_m;
	constexpr static float g_fWorldMaxZ = +500_m;

private:
	inline static std::string g_strSceneBasePath = "../Resources/Scenes";


};

template<typename T> requires std::derived_from<T, IGameObject>
void Scene::AddObject(std::shared_ptr<T> pObj)
{
	if (!pObj) {
		return;
	}

	m_World.Add<T>(pObj);

	if (!m_bSpatialRuntimeRegistrationEnabled) {
		return;
	}

	if constexpr (SpatialObjectTraits<T>::bSpatial) {
		if constexpr (SpatialObjectTraits<T>::bDynamic) {
			m_World.RegisterSpatialObject<T>(pObj);
			m_World.UpdateSpatial();
		}
		else {
			assert(false && "Runtime static spatial registration is not supported. Add static objects before PostInitialize().");
		}
	}
}

