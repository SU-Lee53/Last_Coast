#include "pch.h"
#include "Scene.h"
#include "TerrainObject.h"
#include "StaticObject.h"
#include "NodeObject.h"
#include "Collider.h"
#include "Zombie.h"
#include "Skybox.h"
#include "Sprite.h"

/////////////////////////////////////////////////////////////////////////////
// SpacePartitionDesc

const GridCell* SpacePartitionDesc::GetCellData(const CellCoord& cdCell) const {
	int32 index = CellToIndex(cdCell.x, cdCell.y);
	if (index >= Cells.size()) {
		return nullptr;
	}

	return &Cells[index];
}

SpacePartitionDesc::CellCoord SpacePartitionDesc::WorldToCellXZ(const Vector3& v3WorldPos) const {
	float fX = (v3WorldPos.x - v2SceneOriginXZ.x) / v2CellSizeXZ.x;
	float fZ = (v3WorldPos.z - v2SceneOriginXZ.y) / v2CellSizeXZ.y;

	CellCoord cd;
	cd.x = (int)std::floor(fX);
	cd.y = (int)std::floor(fZ);
	return cd;
}

int32 SpacePartitionDesc::CellToIndex(uint32 x, uint32 z) const {
	return z * xmui2NumCellsXZ.x + x;
}

void SpacePartitionDesc::Insert(std::shared_ptr<IGameObject> pObj) {
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
/////////////////////////////////////////////////////////////////////////////
// Scene


void Scene::InitializeObjects()
{
	if (m_pPlayer) {
		m_pPlayer->Initialize();
	}

	if (m_pTerrain) {
		m_pTerrain->Initialize();
	}

	for (auto& obj : m_pGameObjects) {
		obj->Initialize();
	}
}

void Scene::CleanUp()
{
	m_pGameObjects.clear();
	m_pSprites.clear();
	m_pLights.clear();

	m_pPlayer.reset();
	m_pTerrain.reset();
	m_pSkybox.reset();
}

void Scene::PostInitialize()
{
	InitializeObjects();
	GenerateSceneBound();


	auto [v3SceneMin, v3SceneMax] = ::GetMinMaxFromAABB(m_xmSceneBound);
	float fSceneWidth = v3SceneMax.x - v3SceneMin.x;
	float fSceneHeight = v3SceneMax.z - v3SceneMin.z;

	Vector2 v2SceneOriginXZ;
	v2SceneOriginXZ.x = v3SceneMin.x;
	v2SceneOriginXZ.y = v3SceneMin.z;

	Vector2 v2CellSizeXZ;
	v2CellSizeXZ.x = fSceneWidth / 5;
	v2CellSizeXZ.y = fSceneHeight / 5;

	uint32 unCellsX = std::ceil(fSceneWidth / v2CellSizeXZ.x) + 1;
	uint32 unCellsZ = std::ceil(fSceneHeight / v2CellSizeXZ.y) + 1;

	CellPartition(v2SceneOriginXZ, v2CellSizeXZ, unCellsX, unCellsZ);

	//if (m_pTerrain) {
	//	Vector3 v3TerrainPos = m_pTerrain->GetComponent<Transform>()->GetPosition();
	//	uint32 unComponents = std::sqrt(m_pTerrain->GetTerrainComponents().size());
	//	Vector2 v2CellSize = m_pTerrain->GetTerrainComponents()[0]->GetComponentSize();
	//	CellPartition(Vector2{v3TerrainPos.x, v3TerrainPos.z}, v2CellSize, unComponents, unComponents);
	//}
	//else {
	//	auto [v3SceneMin, v3SceneMax] = ::GetMinMaxFromAABB(m_xmSceneBound);
	//	float fSceneWidth = v3SceneMax.x - v3SceneMin.x;
	//	float fSceneHeight = v3SceneMax.z - v3SceneMin.z;

	//	Vector2 v2SceneOriginXZ;
	//	v2SceneOriginXZ.x = v3SceneMin.x;
	//	v2SceneOriginXZ.y = v3SceneMin.z;

	//	Vector2 v2CellSizeXZ;
	//	v2CellSizeXZ.x = fSceneWidth / 5;
	//	v2CellSizeXZ.y = fSceneHeight / 5;

	//	uint32 unCellsX = std::ceil(fSceneWidth / v2CellSizeXZ.x) + 1;
	//	uint32 unCellsZ = std::ceil(fSceneHeight / v2CellSizeXZ.y) + 1;

	//	CellPartition(v2SceneOriginXZ, v2CellSizeXZ, unCellsX, unCellsZ);
	//}

	if (m_pPlayer) {
		m_pPlayer->GetTransform()->SetPosition(m_xmSceneBound.Center);
	}


	for (auto& obj : m_pGameObjects) {
		auto pZombie = std::dynamic_pointer_cast<Zombie>(obj);
		if (pZombie) {
			//pZombie->SetPosition(AI->GetNavMesh()->GetRandomPoint()); // Transform + AIAgent 동시에
			pZombie->SetPosition(m_xmSceneBound.Center); // Transform + AIAgent 동시에
			pZombie->SetTarget(m_pPlayer);  // shared_ptr 전달
		}
	}
}

void Scene::PreProcessInput()
{
	// TODO : Cache last frame world transform
	// Reason : to generate motion vector
}

void Scene::PostProcessInput()
{
	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
	}

	for (auto& obj : m_pGameObjects) {
		obj->ProcessInput();
	}
}

void Scene::PreUpdate()
{
	if (m_pPlayer) {
		m_pPlayer->PreUpdate();
	}

	for (auto& obj : m_pGameObjects) {
		obj->PreUpdate();
	}
}

void Scene::FixedUpdate()
{
	// Component Update
	if (m_pPlayer) {
		m_pPlayer->Update();
	}

	if (m_pTerrain) {
		m_pTerrain->Update();
	}

	for (auto& obj : m_pGameObjects) {
		obj->Update();
	}

	CheckCollision();
}

void Scene::PostUpdate()
{
	// Post Update
	if (m_pPlayer) {
		m_pPlayer->PostUpdate();
	}

	if (m_pTerrain) {
		m_pTerrain->PostUpdate();
	}

	for (auto& obj : m_pGameObjects) {
		obj->PostUpdate();
	}

	if (m_pSkybox) {
		m_pSkybox->Update();
	}

}

void Scene::PrepareRender()
{
	if (m_pPlayer)
		m_pPlayer->Render();

	for (auto& pObj : m_pGameObjects) {
		pObj->Render();
	}

	for (auto& pSprite : m_pSprites) {
		pSprite->AddToUI(pSprite->GetLayerIndex());
	}
}

void Scene::CheckCollision()
{
	// Broad Phase
	Vector3 v3PlayerPos = m_pPlayer->GetTransform()->GetPosition();
	SpacePartitionDesc::CellCoord cdPlayer = m_SpacePartition.WorldToCellXZ(v3PlayerPos);
	const GridCell* pBroadPhaseResult = m_SpacePartition.GetCellData(cdPlayer);
	if (!pBroadPhaseResult) {
		return;
	}
	
	const std::shared_ptr<PlayerCollider> playerCollider = m_pPlayer->GetComponent<PlayerCollider>();
	if (!playerCollider) {
		return;
	}


	// Player vs StaticObject
	for (const auto& pObj : pBroadPhaseResult->pObjectsInCell) {
		const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>();
		bool bResult = playerCollider->CheckCollision(pCollider);
		if (bResult) {
			CollisionResult result1(m_pPlayer , pObj);
			CollisionResult result2(pObj, m_pPlayer);
			if (!m_pCollisionPairs.contains(result1) || !m_pCollisionPairs.contains(result2)) {
				// Begin Overlap
				m_pPlayer->OnBeginCollision(result1);
				pObj->OnBeginCollision(result2);

				m_pCollisionPairs.insert(result1);
				m_pCollisionPairs.insert(result2);
			}
			else {
				// While Overlap
				m_pPlayer->OnWhileCollision(CollisionResult(m_pPlayer, pObj));
				pObj->OnWhileCollision(CollisionResult(pObj, m_pPlayer));
			}
		}
		else {
			// End Overlap
			CollisionResult result1(m_pPlayer, pObj);
			CollisionResult result2(pObj, m_pPlayer);
			if (m_pCollisionPairs.contains(result1) || m_pCollisionPairs.contains(result2)) {
				m_pPlayer->OnEndCollision(result1);
				pObj->OnEndCollision(result2);

				m_pCollisionPairs.erase(result1);
				m_pCollisionPairs.erase(result2);
			}
		}
	}

	// ── 좀비 vs 정적 오브젝트 ──────────────────────────────────────────────
	for (auto& obj : m_pGameObjects) {
		auto pZombie = std::dynamic_pointer_cast<Zombie>(obj);
		if (!pZombie)
			continue;

		auto pZombieCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pZombieCollider)
			continue;

		Vector3 v3ZombiePos = pZombie->GetTransform()->GetPosition();
		SpacePartitionDesc::CellCoord cdZombie = m_SpacePartition.WorldToCellXZ(v3ZombiePos);
		const GridCell* pZombieBroadPhase = m_SpacePartition.GetCellData(cdZombie);
		if (!pZombieBroadPhase)
			continue;

		const PlayerCollider& zombieCollider = *pZombieCollider;
		for (const auto& pObj : pZombieBroadPhase->pObjectsInCell) {
			const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>();
			if (!pCollider)
				continue;

			bool bResult = zombieCollider.CheckCollision(pCollider);
			if (bResult) {
				CollisionResult result1(pZombie, pObj);
				CollisionResult result2(pObj, pZombie);
				if (!m_pCollisionPairs.contains(result1) || !m_pCollisionPairs.contains(result2)) {
					pZombie->OnBeginCollision(result1);
					pObj->OnBeginCollision(result2);
					m_pCollisionPairs.insert(result1);
					m_pCollisionPairs.insert(result2);
				}
				else {
					pZombie->OnWhileCollision(CollisionResult(pZombie, pObj));
					pObj->OnWhileCollision(CollisionResult(pObj, pZombie));
				}
			}
			else {
				CollisionResult result1(pZombie, pObj);
				CollisionResult result2(pObj, pZombie);
				if (m_pCollisionPairs.contains(result1) || m_pCollisionPairs.contains(result2)) {
					pZombie->OnEndCollision(result1);
					pObj->OnEndCollision(result2);
					m_pCollisionPairs.erase(result1);
					m_pCollisionPairs.erase(result2);
				}
			}
		}
	}
}

void Scene::GenerateSceneBound()
{
	for (const auto& pObj : m_pGameObjects) {
		const auto& pCollider = pObj->GetComponent<ICollider>();
		if (!pCollider) {
			continue;
		}

		const auto& xmAABB = pCollider->GetAABBFromOBBWorld();
		if (m_xmSceneBound.Center == Vector3(0, 0, 0) && m_xmSceneBound.Extents == Vector3(1, 1, 1)) {
			m_xmSceneBound = xmAABB;
		}
		else {
			BoundingBox::CreateMerged(m_xmSceneBound, m_xmSceneBound, xmAABB);
		}
	}
}

void Scene::CellPartition(const Vector2& v2OriginXZ, const Vector2& v2SizePerCellXZ, uint32 unCellsX, uint32 unCellsZ)
{
	// 1. Generate cells (2.5D)
	m_SpacePartition.v2SceneOriginXZ = v2OriginXZ;
	m_SpacePartition.v2CellSizeXZ = v2SizePerCellXZ;
	m_SpacePartition.xmui2NumCellsXZ = XMUINT2{ unCellsX, unCellsZ };
	m_SpacePartition.Cells.resize(unCellsX * unCellsZ);

	// 2. Check objects in cells
	for (const auto& pObj : m_pGameObjects) {
		m_SpacePartition.Insert(pObj);
	}
}

std::vector<LightData> Scene::MakeLightData() const
{
	std::vector<LightData> lightData;
	lightData.reserve(m_pLights.size());

	for (auto& pLight : m_pLights) {
		lightData.push_back(pLight->MakeCBData());
	}
	return lightData;
}

TerrainHit Scene::QueryTerrainHit(const Vector3& v3WorldPos)
{
	TerrainHit result{};
	if (!m_pTerrain) {
		return result;
	}

	float fHeight = m_pTerrain->GetHeightWorld(v3WorldPos);
	if (v3WorldPos.y <= fHeight) {
		result.bGrounded = true;
		result.fHeight = fHeight;

		result.v3Normal = m_pTerrain->GetNormalWorld(v3WorldPos);
		if (result.v3Normal.LengthSquared() < 0.1f) {
			result.v3Normal = Vector3::Up;
		}

		result.fPenetrationDepth = fHeight - v3WorldPos.y;
	}

	return result;
}

void Scene::BuildLights()
{
	m_pLights.reserve(1);

	auto pLight = std::make_shared<DirectionalLight>();
	{
		pLight->m_v3Color = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Direction = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Position = Vector3{ 100.f, 10000.f, 100.f };
		pLight->m_fIntensity = 0.2;

		pLight->m_v3Direction.Normalize();
	}

	m_pLights.push_back(pLight);
}

HRESULT Scene::LoadFromFiles(const std::string& strFileName)
{
	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);

	std::ifstream inFile{ strFilePath, std::ios::binary };
	if (!inFile) {
		__debugbreak();
		return E_INVALIDARG;
	}

	//std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	//nlohmann::json j = nlohmann::json::from_bson(bson);

	nlohmann::json jScene = nlohmann::json::parse(inFile);

	if (jScene.contains("StaticMeshActors")) {
		for (const auto& jObject : jScene["StaticMeshActors"]) {
			std::shared_ptr<IGameObject> pObj = std::make_shared<StaticObject>();
			pObj->SetName(jObject["ActorName"].get<std::string>());

			auto matrixData = jObject["Transform"]["WorldMatrix"].get<std::vector<float>>();
			Matrix mtxWorldMatrix(matrixData.data());
			pObj->GetTransform()->SetWorldMatrix(mtxWorldMatrix);

			std::string strMeshName = jObject["MeshName"].get<std::string>();
			auto pMeshObject = MODEL->LoadOrGet(strMeshName)->CopyObject<NodeObject>();
			pObj->SetChild(pMeshObject);

			m_pGameObjects.push_back(pObj);
		}
	}

	// Lights 로드

	if (jScene.contains("Lights")) {
		for (const auto& jLight : jScene["Lights"]) {
			std::string strType = jLight["Type"].get<std::string>();

			// PointLight
			if (strType == "PointLight") {
				std::shared_ptr<PointLight> pLight = std::make_shared<PointLight>();
				// Transform (Position)
				auto matrixData = jLight["Transform"]["WorldMatrix"].get<std::vector<float>>();
				Matrix mtxWorldMatrix(matrixData.data());
				Vector3 v3Position(mtxWorldMatrix._41, mtxWorldMatrix._42, mtxWorldMatrix._43);
				pLight->m_v3Position = v3Position;

				// Color와 Intensity로 Diffuse 계산
				float intensity = jLight["Intensity"].get<float>();
				float colorX = jLight["Color"]["X"].get<float>();
				float colorY = jLight["Color"]["Y"].get<float>();
				float colorZ = jLight["Color"]["Z"].get<float>();

				//pLight->m_v4Diffuse.x = colorX * intensity;
				//pLight->m_v4Diffuse.y = colorY * intensity;
				//pLight->m_v4Diffuse.z = colorZ * intensity;
				//pLight->m_v4Diffuse.w = 1.0f;

				pLight->m_v3Color = Vector3(colorX, colorY, colorZ);
				pLight->m_fIntensity = intensity;

				// Range & Attenuation
				pLight->m_fRange = jLight["Range"].get<float>();
				pLight->m_fAttenuation0 = jLight["Attenuation0"].get<float>();
				pLight->m_fAttenuation1 = jLight["Attenuation1"].get<float>();
				pLight->m_fAttenuation2 = jLight["Attenuation2"].get<float>();

				m_pLights.push_back(pLight);
			}
			// SpotLight
			else if (strType == "SpotLight") {
				std::shared_ptr<SpotLight> pLight = std::make_shared<SpotLight>();
				// Transform (Position)
				auto matrixData = jLight["Transform"]["WorldMatrix"].get<std::vector<float>>();
				Matrix mtxWorldMatrix(matrixData.data());
				Vector3 v3Position(mtxWorldMatrix._41, mtxWorldMatrix._42, mtxWorldMatrix._43);
				pLight->m_v3Position = v3Position;

				// Direction
				float dirX = jLight["Direction"]["X"].get<float>();
				float dirY = jLight["Direction"]["Y"].get<float>();
				float dirZ = jLight["Direction"]["Z"].get<float>();
				pLight->m_v3Direction = Vector3(dirX, dirY, dirZ);

				// Color와 Intensity로 Diffuse 계산
				float intensity = jLight["Intensity"].get<float>();
				float colorX = jLight["Color"]["X"].get<float>();
				float colorY = jLight["Color"]["Y"].get<float>();
				float colorZ = jLight["Color"]["Z"].get<float>();

				//pLight->m_v4Diffuse.x = colorX * intensity;
				//pLight->m_v4Diffuse.y = colorY * intensity;
				//pLight->m_v4Diffuse.z = colorZ * intensity;
				//pLight->m_v4Diffuse.w = 1.0f;

				pLight->m_v3Color = Vector3(colorX, colorY, colorZ);
				pLight->m_fIntensity = intensity;

				// Range & Attenuation
				pLight->m_fRange = jLight["Range"].get<float>();
				pLight->m_fAttenuation0 = jLight["Attenuation0"].get<float>();
				pLight->m_fAttenuation1 = jLight["Attenuation1"].get<float>();
				pLight->m_fAttenuation2 = jLight["Attenuation2"].get<float>();

				// Falloff
				pLight->m_fFalloff = jLight["Falloff"].get<float>();

				// Cone Angles
				pLight->m_fPhi = jLight["Theta"].get<float>();
				pLight->m_fTheta = jLight["Phi"].get<float>();
				m_pLights.push_back(pLight);
			}
		}
	}

}
