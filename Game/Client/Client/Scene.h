#pragma once
#include "GameObject.h"
#include "Player.h"	// Includes GameObject
#include "Camera.h"
#include "Light.h"

class TerrainObject;
class Skybox;
class Sprite;

struct GridCell {
	std::vector<std::shared_ptr<IGameObject>> pObjectsInCell;
};

using CollisionPair = std::pair<std::shared_ptr<IGameObject>, std::shared_ptr<IGameObject>>;


struct SpacePartitionDesc {
	using CellCoord = XMINT2;

	std::vector<GridCell> Cells;
	Vector2 v2SceneOriginXZ;
	Vector2 v2CellSizeXZ;
	XMUINT2 xmui2NumCellsXZ;

	const GridCell* GetCellData(const CellCoord& cdCell) const;

	CellCoord WorldToCellXZ(const Vector3& v3WorldPos) const;

	int32 CellToIndex(uint32 x, uint32 z) const;

	void Insert(std::shared_ptr<IGameObject> pObj);

};

class Scene {
	friend class SceneManager;

public:
	virtual void BuildObjects() = 0;
	virtual void BuildLights();

public:
	void AddObject(std::shared_ptr<IGameObject> pObj) {
		m_pGameObjects.push_back(pObj);
	}

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

	void CheckCollision();

	void GenerateSceneBound();
	void CellPartition(const Vector2& v3OriginXZ, const Vector2& v2SizePerCellXZ, uint32 unCellsX, uint32 unCellsZ);

	virtual void SyncSceneWithServer() {}

public:
	const std::shared_ptr<IPlayer>& GetPlayer() const { return m_pPlayer; }
	const std::shared_ptr<TerrainObject>& GetTerrain() const { return m_pTerrain; }
	const std::shared_ptr<Skybox>& GetSkybox() const { return m_pSkybox; }
	const std::shared_ptr<Camera>& GetCamera() const { return m_pPlayer->GetCamera(); }
	const std::vector<std::shared_ptr<IGameObject>>& GetObjectsInScene() const { return m_pGameObjects; }
	const std::vector<std::shared_ptr<Light>>& GetLightsInScene() const { return m_pLights; }
	const Vector4& GetGlobalAmbient() const { return m_v4GlobalAmbient; }

	const SpacePartitionDesc& GetSpacePartitionDesc() const { return m_SpacePartition; }

	std::vector<LightData> MakeLightData() const;

	TerrainHit QueryTerrainHit(const Vector3& v3WorldPos);

protected:
	void InitializeObjects();

protected:
	std::vector<std::shared_ptr<IGameObject>>	m_pGameObjects = {};
	std::vector<std::shared_ptr<Sprite>>		m_pSprites;
	std::vector<std::shared_ptr<Light>>			m_pLights = {};
	
	std::shared_ptr<IPlayer>					m_pPlayer = nullptr;
	std::shared_ptr<TerrainObject>				m_pTerrain = nullptr;
	std::shared_ptr<Skybox>						m_pSkybox = nullptr;

	//std::vector<GridCell> m_GridCells;
	SpacePartitionDesc m_SpacePartition{};
	BoundingBox m_xmSceneBound{};

	std::unordered_set<CollisionResult> m_pCollisionPairs;

	Vector4 m_v4GlobalAmbient;

public:
	constexpr static float g_fWorldMinX = -500_m;
	constexpr static float g_fWorldMaxX = +500_m;

	constexpr static float g_fWorldMinZ = -500_m;
	constexpr static float g_fWorldMaxZ = +500_m;

private:
	inline static std::string g_strSceneBasePath = "../Resources/Scenes";

};

