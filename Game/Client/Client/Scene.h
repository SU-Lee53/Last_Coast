#pragma once
#include "Player.h"	// Includes GameObject
#include "Sprite.h"	// Includes GameObject
#include "Camera.h"
#include "Light.h"

class TerrainObject;

struct GridCell {
	std::vector<std::shared_ptr<IGameObject>> pObjectsInCell;
};

struct SpacePartitionDesc {
	using CellCoord = XMINT2;

	std::vector<GridCell> Cells;
	Vector2 v2SceneOriginXZ;
	Vector2 v2CellSizeXZ;
	XMUINT2 xmui2NumCellsXZ;

	const GridCell* GetCellData(const CellCoord& cdCell) const {
		int32 index = CellToIndex(cdCell.x, cdCell.y);
		if (index >= Cells.size()) {
			return nullptr;
		}

		return &Cells[index];
	}

	CellCoord WorldToCellXZ(const Vector3& v3WorldPos) const {
		float fX = (v3WorldPos.x - v2SceneOriginXZ.x) / v2CellSizeXZ.x;
		float fZ = (v3WorldPos.z - v2SceneOriginXZ.y) / v2CellSizeXZ.y;
		
		CellCoord cd;
		cd.x = (int)std::floor(fX);
		cd.y = (int)std::floor(fZ);
		return cd;
	}

	int32 CellToIndex(uint32 x, uint32 z) const {
		return z * xmui2NumCellsXZ.x + x;
	}

	void Insert(std::shared_ptr<IGameObject> pObj) {
		const BoundingOrientedBox& xmOBB = pObj->GetComponent<ICollider>()->GetOBBWorld();
		
		auto [v3Min, v3Max] = GetMinMaxFromOBB(xmOBB);
		CellCoord cdMin = WorldToCellXZ(v3Min);
		CellCoord cdMax = WorldToCellXZ(v3Max);

		cdMin.x = std::clamp(cdMin.x, 0, (int32)xmui2NumCellsXZ.x);
		cdMin.y = std::clamp(cdMin.y, 0, (int32)xmui2NumCellsXZ.y);

		cdMax.x = std::clamp(cdMax.x, 0, (int32)xmui2NumCellsXZ.x);
		cdMax.y = std::clamp(cdMax.y, 0, (int32)xmui2NumCellsXZ.y);

		for (uint32 x = cdMin.x; x <= cdMax.x; ++x) {
			for (uint32 z = cdMin.y; z <= cdMax.y; ++z) {
				Cells[CellToIndex(x, z)].pObjectsInCell.push_back(pObj);
			}
		}
	}

};

class Scene {
	friend class SceneManager;

public:
	virtual void BuildObjects() = 0;
	virtual void BuildLights() {}

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
	virtual void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList) = 0;

	virtual void OnEnterScene() = 0;
	virtual void OnLeaveScene() = 0;

	void PostInitialize();
	void PreProcessInput();
	void PostProcessInput();
	void PreUpdate();
	void FixedUpdate();
	void PostUpdate();

	void GenerateSceneBound();
	void CellPartition(const Vector2& v3OriginXZ, const Vector2& v2SizePerCellXZ, uint32 unCellsX, uint32 unCellsZ);

	virtual void SyncSceneWithServer() {}

public:
	const std::shared_ptr<Player>& GetPlayer() const { return m_pPlayer; }
	const std::shared_ptr<TerrainObject>& GetTerrain() const { return m_pTerrain; }
	const std::shared_ptr<Camera>& GetCamera() const { return m_pPlayer->GetCamera(); }
	std::vector<std::shared_ptr<IGameObject>>& GetObjectsInScene() { return m_pGameObjects; }

	const SpacePartitionDesc& GetSpacePartitionDesc() const { return m_SpacePartition; }

	CB_LIGHT_DATA MakeLightData();

protected:
	void InitializeObjects();
	void RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

protected:
	std::vector<std::shared_ptr<IGameObject>>	m_pGameObjects = {};
	std::vector<std::shared_ptr<Sprite>>		m_pSprites;
	std::vector<std::shared_ptr<Light>>			m_pLights = {};
	
	std::shared_ptr<Player>						m_pPlayer = nullptr;
	std::shared_ptr<TerrainObject>				m_pTerrain = nullptr;

	//std::vector<GridCell> m_GridCells;
	SpacePartitionDesc m_SpacePartition{};
	BoundingBox m_xmSceneBound{};

	Vector4 m_v4GlobalAmbient;

public:
	constexpr static float g_fWorldMinX = -500_m;
	constexpr static float g_fWorldMaxX = +500_m;

	constexpr static float g_fWorldMinZ = -500_m;
	constexpr static float g_fWorldMaxZ = +500_m;

private:
	inline static std::string g_strSceneBasePath = "../Resources/Scenes";

};

