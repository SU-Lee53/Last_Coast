#include "pch.h"
#include "Scene.h"
#include "TerrainObject.h"
#include "StaticObject.h"
#include "NodeObject.h"
#include "Collider.h"

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

void Scene::RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_pPlayer)
		m_pPlayer->Render(pd3dCommandList);

	for (auto& pObj : m_pGameObjects) {
		pObj->Render(pd3dCommandList);
	}

	for (auto& pSprite : m_pSprites) {
		pSprite->AddToUI(pSprite->GetLayerIndex());
	}
}

void Scene::PostInitialize()
{
	InitializeObjects();
	GenerateSceneBound();
	if (m_pTerrain) {
		Vector3 v3TerrainPos = m_pTerrain->GetComponent<Transform>()->GetPosition();
		uint32 unComponents = std::sqrt(m_pTerrain->GetTerrainComponents().size());
		Vector2 v2CellSize = m_pTerrain->GetTerrainComponents()[0]->GetComponentSize();
		CellPartition(Vector2{v3TerrainPos.x, v3TerrainPos.z}, v2CellSize, unComponents, unComponents);
	}
	else {
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
	}

	if (m_pPlayer) {
		m_pPlayer->GetTransform()->SetPosition(m_xmSceneBound.Center);
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
}

void Scene::CheckCollision() 
{
	// 1. Broad Phase
	Vector3 v3PlayerPos = m_pPlayer->GetTransform()->GetPosition();
	SpacePartitionDesc::CellCoord cdPlayer = m_SpacePartition.WorldToCellXZ(v3PlayerPos);
	int32 cellIndex = m_SpacePartition.CellToIndex(cdPlayer.x, cdPlayer.y);
	const GridCell* pBroadPhaseResult = m_SpacePartition.GetCellData(cdPlayer);
	if (!pBroadPhaseResult) {
		return;
	}
	
	//std::vector<ICollider> CollidersInCell;
	//CollidersInCell.reserve(pBroadPhaseResult.size());
	//std::transform(pBroadPhaseResult.begin(), pBroadPhaseResult.end(), std::back_inserter(CollidersInCell),
	//	[](const std::shared_ptr<IGameObject> pOBj) {
	//		return *pOBj->GetComponent<StaticCollider>();
	//});

	// 2. Narrow Phase
	const PlayerCollider& playerCollider = *m_pPlayer->GetComponent<PlayerCollider>();
	for (const auto& pObj : pBroadPhaseResult->pObjectsInCell) {
		const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>();
		bool bResult = playerCollider.CheckCollision(pCollider);
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

CB_LIGHT_DATA Scene::MakeLightData()
{
	CB_LIGHT_DATA lightData;

	for (int i = 0; i < m_pLights.size(); ++i) {
		lightData.gLights[i] = m_pLights[i]->MakeLightData();
	}

	lightData.gcGlobalAmbientLight = Vector4(1.f, 1.f, 1.f, 1.f);
	lightData.gnLights = m_pLights.size();

	return lightData;
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
	//nlohmann::json j = nlohmann::json::from_bson(bson);;

	nlohmann::json jScene = nlohmann::json::parse(inFile);

	if (jScene.contains("StaticMeshActors")) {
		for (const auto& jObject : jScene["StaticMeshActors"]) {
			std::shared_ptr<IGameObject> pObj = std::make_shared<StaticObject>();
			pObj->SetName(jObject["ActorName"].get<std::string>());

			auto matrixData = jObject["Transform"]["WorldMatrix"].get<std::vector<float>>();
			Matrix M4x4WorldMatrix(matrixData.data());
			pObj->GetTransform()->SetWorldMatrix(M4x4WorldMatrix);

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
				Matrix M4x4WorldMatrix(matrixData.data());
				Vector3 v3Position(M4x4WorldMatrix._41, M4x4WorldMatrix._42, M4x4WorldMatrix._43);
				pLight->m_v3Position = v3Position;

				// Color와 Intensity로 Diffuse 계산
				float intensity = jLight["Intensity"].get<float>();
				float colorX = jLight["Color"]["X"].get<float>();
				float colorY = jLight["Color"]["Y"].get<float>();
				float colorZ = jLight["Color"]["Z"].get<float>();

				pLight->m_v4Diffuse.x = colorX * intensity;
				pLight->m_v4Diffuse.y = colorY * intensity;
				pLight->m_v4Diffuse.z = colorZ * intensity;
				pLight->m_v4Diffuse.w = 1.0f;

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
				Matrix M4x4WorldMatrix(matrixData.data());
				Vector3 v3Position(M4x4WorldMatrix._41, M4x4WorldMatrix._42, M4x4WorldMatrix._43);
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

				pLight->m_v4Diffuse.x = colorX * intensity;
				pLight->m_v4Diffuse.y = colorY * intensity;
				pLight->m_v4Diffuse.z = colorZ * intensity;
				pLight->m_v4Diffuse.w = 1.0f;

				// Range & Attenuation
				pLight->m_fRange = jLight["Range"].get<float>();
				pLight->m_fAttenuation0 = jLight["Attenuation0"].get<float>();
				pLight->m_fAttenuation1 = jLight["Attenuation1"].get<float>();
				pLight->m_fAttenuation2 = jLight["Attenuation2"].get<float>();

				// Falloff
				pLight->m_fFalloff = jLight["Falloff"].get<float>();

				// Cone Angles
				pLight->m_fTheta = jLight["Theta"].get<float>();
				pLight->m_fPhi = jLight["Phi"].get<float>();

				m_pLights.push_back(pLight);
			}
		}
	}

}
