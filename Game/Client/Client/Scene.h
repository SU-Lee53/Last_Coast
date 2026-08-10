#pragma once
#include "World.h"
#include "GameObject.h"
#include "ToneMappingVolume.h"
#include "PostProcessingVolume.h"

#include "StaticObject.h"
#include "Zombie.h"
#include "WeaponObject.h"
#include "CrashDebris.h"
#include "GrenadeProjectile.h"
#include "WaterGridObject.h"

#include "Player.h"	// Includes GameObject
#include "Camera.h"
#include "Light.h"
#include "UIBoard.h"

class TerrainComponent;
class TerrainObject;
class Skybox;
class EventSequence;
class HelicopterObject;
//class Sprite;

using CollisionPair = std::pair<std::shared_ptr<IGameObject>, std::shared_ptr<IGameObject>>;

//#define TIME_RECORD

class Scene {
	friend class SceneManager;

	using WorldType = World<
		NetworkOwnerThirdPersonPlayer,
		NetworkRemoteThirdPersonPlayer,
		StaticObject, WeaponObject,
		Zombie,
		WaterGridObject,
		HelicopterObject,
		CrashDebris,
		GrenadeProjectile>;

public:
	virtual ~Scene() = default;

	virtual void BuildObjects() = 0;
	virtual void BuildLights();


	// Complete building scene of async changable scene
	virtual void FinalizeBuild() {};

public:
	template<typename T> requires std::derived_from<T, IGameObject>
	void AddObject(std::shared_ptr<T> pObj);

	template<typename T> requires std::derived_from<T, IGameObject>
	void RemoveObject(std::shared_ptr<T> pObj);

	template<typename... Objs, 
		typename = std::enable_if_t<(std::is_same_v<Objs, std::shared_ptr<IGameObject>> && ...)>>
	void AddObjects(Objs... pObjs) {
		(m_pGameObjects.push_back(std::forward<Objs>(pObjs)), ...);
	}

	HRESULT LoadFromFiles(const std::string& strFileName);
	// 좀비 스폰 포인트를 별도 JSON 파일에서 로드 (씬과 분리)
	HRESULT LoadZombieSpawnPoints(const std::string& strFileName);
	// 헬기 비행 경로점을 별도 JSON 파일에서 로드 (씬과 분리, 순서 보존)
	HRESULT LoadHeliPath(const std::string& strFileName);
	// 구조 헬기(착륙) 비행 경로점을 별도 JSON에서 로드 (추락 경로와 분리)
	HRESULT LoadHeliArrivePath(const std::string& strFileName);


public:
	virtual void ProcessInput() = 0;
	virtual void Update() = 0;
	void CleanUp();

	virtual void OnEnterScene() = 0;
	virtual void OnLeaveScene() = 0;

	void PostInitialize();
	void PreProcessInput();
	virtual void PostProcessInput(); // GameScene이 전원 로딩 대기 중 입력 차단용으로 override
	void PreUpdate();
	void FixedUpdate();
	void PostUpdate();
	void PrepareRender();

	void GenerateSceneBound();

	void CheckCollision();
	void RemoveInvalidCollisionSet(const SpatialQueryResult& playerBroadPhaseResult);

	virtual void SyncSceneWithServer() {}

public:
	const WorldType& GetWorld() const { return m_World; }
	const std::shared_ptr<IPlayer>& GetPlayer() const { return m_pPlayer; }
	const std::shared_ptr<TerrainObject>& GetTerrain() const { return m_pTerrain; }
	const std::shared_ptr<Skybox>& GetSkybox() const { return m_pSkybox; }
	const std::shared_ptr<Camera>& GetCamera() const { return m_pMainCamera; }
	const std::vector<std::shared_ptr<Light>>& GetLightsInScene() const { return m_pLights; }
	std::shared_ptr<DirectionalLight> GetSunLight() const;
	const Vector4& GetGlobalAmbient() const { return m_v4GlobalAmbient; }
	void SetGlobalAmbient(const Vector4& v4Ambient) { m_v4GlobalAmbient = v4Ambient; }
	const std::unique_ptr<UIBoard>& GetUIBoard() const { return m_pUIBoard; }
	const std::vector<Vector3>& GetZombieSpawnPoints() const { return m_v3ZombieSpawnPoints; }
	const std::vector<Vector3>& GetHeliPath() const { return m_v3HeliPath; }
	const std::vector<Vector3>& GetHeliArrivePath() const { return m_v3HeliArrivePath; }

	const ToneMappingVolume& GetToneMappingVolume() const { return m_ToneMappingVolume; }
	ToneMappingVolume& GetToneMappingVolume() { return m_ToneMappingVolume; } // 게임 이벤트 런타임 조정용
	const PostProcessingVolume& GetPostProcessingVolume() const { return m_PostProcessingVolume; }
	PostProcessingVolume& GetPostProcessingVolume() { return m_PostProcessingVolume; } // 게임 이벤트 런타임 조정용

	bool IsGravityOn() const { return m_bEnableGravity; }

	std::vector<LightData> MakeLightData() const;

	TerrainHit QueryTerrainHit(const Vector3& v3WorldPos);

	std::shared_ptr<Camera> SwapCamera(std::shared_ptr<Camera>& pNewCamera);

	// 게임 이벤트가 후속 이벤트를 띄울 때 사용 (예: 헬기 추락 폭발 → 잔해 화재)
	const std::shared_ptr<EventSequence>& GetEventSequence() const { return m_pEventSequence; }

	// 컷씬용 임시 오브젝트(월드/스폐셜 미등록). PrepareRender에서 직접 Render 호출.
	// 이벤트(예: HelicopterCrashEvent)가 매 프레임 Transform 갱신 후 사용.
	void SetCinematicProp(const std::shared_ptr<IGameObject>& pProp) { m_pCinematicProp = pProp; }
	void ClearCinematicProp() { m_pCinematicProp = nullptr; }
	const std::shared_ptr<IGameObject>& GetCinematicProp() const { return m_pCinematicProp; }

	// 컷씬(시네마틱) 게임플레이 정지: 입력/플레이어/좀비(월드) 업데이트를 멈춘다.
	// 이벤트 시퀀스·스카이박스(시간/태양)는 계속 구동되어 연출은 진행됨.
	// 중첩 컷씬 대비 깊이 카운터(컷씬 이벤트가 진입 시 Push, 종료 시 Pop).
	void PushCinematic(); // 첫 진입(0→1) 시 조준 해제 등 정리 — Scene.cpp
	void PopCinematic()  { if (m_nCinematicDepth > 0) --m_nCinematicDepth; }
	bool IsCinematicActive() const { return m_nCinematicDepth > 0; }

	// 탈출 컷씬: 전 플레이어(로컬+리모트) 렌더 숨김 토글
	void SetHideCharacters(bool bHide) { m_bHideCharacters = bHide; }
	bool IsHidingCharacters() const { return m_bHideCharacters; }

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
	std::shared_ptr<Camera>						m_pMainCamera = nullptr;
	std::shared_ptr<TerrainObject>				m_pTerrain = nullptr;
	std::shared_ptr<Skybox>						m_pSkybox = nullptr;
	std::shared_ptr<EventSequence>				m_pEventSequence = nullptr;
	std::shared_ptr<IGameObject>				m_pCinematicProp = nullptr;	// 컷씬 임시 오브젝트

	BoundingBox m_xmSceneBound{};
	std::vector<Vector3> m_v3ZombieSpawnPoints;	// 언리얼에서 내보낸 좀비 스폰 위치 (cm)
	std::vector<Vector3> m_v3HeliPath;			// 언리얼에서 내보낸 헬기 추락 경로점 (cm, 순서대로)
	std::vector<Vector3> m_v3HeliArrivePath;	// 언리얼에서 내보낸 구조 헬기(착륙) 경로점 (cm, 순서대로)
	std::unique_ptr<UIBoard> m_pUIBoard{};
	std::unordered_set<CollisionResult> m_pCollisionPairs;
	Vector4 m_v4GlobalAmbient;
	bool m_bEnableGravity = true;
	int  m_nCinematicDepth = 0;	// >0 이면 컷씬 중(게임플레이 정지)
	bool m_bHideCharacters = false;	// 탈출 컷씬 동안 전 플레이어 렌더 숨김

	std::unordered_map<int, std::shared_ptr<NetworkRemoteThirdPersonPlayer>> m_RemotePlayers;
	

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

template<typename T> requires std::derived_from<T, IGameObject>
void Scene::RemoveObject(std::shared_ptr<T> pObj)
{
	if (!pObj) {
		return;
	}

	if constexpr (SpatialObjectTraits<T>::bSpatial) {
		if constexpr (SpatialObjectTraits<T>::bDynamic) {
			m_World.UnregisterSpatialObject<T>(pObj);
			// m_World.UpdateSpatial(); // UpdateSpatial is typically called once per frame, not on every remove.
		}
	}

	m_World.Remove<T>(pObj);
}

