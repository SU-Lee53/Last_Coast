#include "pch.h"
#include "Scene.h"
#include "TerrainObject.h"
#include "NodeObject.h"
#include "Collider.h"
#include "Skybox.h"
#include <queue>

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

	m_World.IntiializeObjects();

	//for (auto& obj : m_pStaticObjects) {
	//	obj->Initialize();
	//}

	//for (auto& obj : m_pDynamicObjects) {
	//	obj->Initialize();
	//}
}

void Scene::CleanUp()
{
	m_bSpatialRuntimeRegistrationEnabled = false;

	m_pCollisionPairs.clear();

	m_World.ClearAll();
	m_pLights.clear();

	m_pPlayer.reset();
	m_pTerrain.reset();
	m_pSkybox.reset();
}

void Scene::PostInitialize()
{
	InitializeObjects();
	GenerateSceneBound();

	/* Old xz grid space partition */
	//	if (m_pTerrain) {
	//		Vector3 v3TerrainPos = m_pTerrain->GetComponent<Transform>()->GetPosition();
	//		uint32 unComponents = std::sqrt(m_pTerrain->GetTerrainComponents().size());
	//		Vector2 v2CellSize = m_pTerrain->GetTerrainComponents()[0]->GetComponentSize();
	//		CellPartition(Vector2{ v3TerrainPos.x, v3TerrainPos.z }, v2CellSize, unComponents, unComponents);
	//	}
	//	else {
	//		auto [v3SceneMin, v3SceneMax] = ::GetMinMaxFromAABB(m_xmSceneBound);
	//		float fSceneWidth = v3SceneMax.x - v3SceneMin.x;
	//		float fSceneHeight = v3SceneMax.z - v3SceneMin.z;
	//	
	//		Vector2 v2SceneOriginXZ;
	//		v2SceneOriginXZ.x = v3SceneMin.x;
	//		v2SceneOriginXZ.y = v3SceneMin.z;
	//	
	//		Vector2 v2CellSizeXZ;
	//		v2CellSizeXZ.x = fSceneWidth / 5;
	//		v2CellSizeXZ.y = fSceneHeight / 5;
	//	
	//		uint32 unCellsX = std::ceil(fSceneWidth / v2CellSizeXZ.x) + 1;
	//		uint32 unCellsZ = std::ceil(fSceneHeight / v2CellSizeXZ.y) + 1;
	//	
	//		CellPartition(v2SceneOriginXZ, v2CellSizeXZ, unCellsX, unCellsZ);
	//	}

	// Space Partition
	{
		m_World.RegisterSpatialObjectsByMobility<false>();

		// 3. Build Static Grid
		SpatialGridBuildDesc gridDesc{};

		if (m_pTerrain) {
			Vector3 v3TerrainPos = m_pTerrain->GetComponent<Transform>()->GetPosition();
			const uint32 unComponents = static_cast<uint32>(std::sqrt(m_pTerrain->GetTerrainComponents().size()));
			Vector2 v2CellSize = m_pTerrain->GetTerrainComponents()[0]->GetComponentSize();

			gridDesc.v2OriginXZ = Vector2{ v3TerrainPos.x, v3TerrainPos.z };
			gridDesc.v2CellSizeXZ = v2CellSize;
			gridDesc.xmui2NumCellsXZ = XMUINT2{ unComponents, unComponents };
			gridDesc.fMinY = m_pTerrain->GetMinHeight();
			gridDesc.fMaxY = m_pTerrain->GetMaxHeight();
		}
		else {
			auto [v3SceneMin, v3SceneMax] = ::GetMinMaxFromAABB(m_xmSceneBound);

			const float fSceneWidth = v3SceneMax.x - v3SceneMin.x;
			const float fSceneDepth = v3SceneMax.z - v3SceneMin.z;

			constexpr float fDefaultCellSize = 100_m;

			const uint32 unCellsX = std::max(1u, static_cast<uint32>(std::ceil(fSceneWidth / fDefaultCellSize)));
			const uint32 unCellsZ = std::max(1u, static_cast<uint32>(std::ceil(fSceneDepth / fDefaultCellSize)));

			gridDesc.v2OriginXZ = Vector2{ v3SceneMin.x, v3SceneMin.z };
			gridDesc.v2CellSizeXZ = Vector2{ fDefaultCellSize, fDefaultCellSize };
			gridDesc.xmui2NumCellsXZ = XMUINT2{ unCellsX, unCellsZ };
			gridDesc.fMinY = v3SceneMin.y;
			gridDesc.fMaxY = v3SceneMax.y;
		}

		if (m_pTerrain) {
			for (auto& pComponent : m_pTerrain->GetTerrainComponents()) {
				m_World.GetSpatial().RegisterTerrain(
					pComponent.get(),
					SPATIAL_TERRAIN |
					SPATIAL_COLLIDABLE |
					SPATIAL_RAY_TARGET |
					SPATIAL_CAST_SHADOW
				);
			}
		}

		for (auto& pLight : m_pLights) {
			if (std::dynamic_pointer_cast<DirectionalLight>(pLight)) {
				continue;
			}

			m_World.GetSpatial().RegisterLight(
				pLight.get(),
				SPATIAL_LIGHT,
				true
			);
		}

		// 2. Update Proxy bounds
		m_World.UpdateSpatial();
		m_World.GetSpatial().BuildStaticGrid(gridDesc);
	}

	if (m_pPlayer) {
		m_pPlayer->GetTransform()->SetPosition(m_xmSceneBound.Center);
	}

	for (auto& pZombie : m_World.GetObjects<Zombie>()) {
		pZombie->SetPosition(AI->GetNavMesh()->GetRandomPoint()); // Transform + AIAgent 동시에
		//pZombie->SetPosition(m_xmSceneBound.Center); // Transform + AIAgent 동시에
		pZombie->SetTarget(m_pPlayer);  // shared_ptr 전달
	}

	// Register Spatial Dynamic
	{
		m_World.RegisterSpatialObjectsByMobility<true>();
		m_World.UpdateSpatial();
	}

	m_bSpatialRuntimeRegistrationEnabled = true;
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

	m_World.PostProcessInput();
}

void Scene::PreUpdate()
{
	if (m_pPlayer) {
		m_pPlayer->PreUpdate();
	}

	m_World.PreUpdate();
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

	m_World.FixedUpdate();
	m_World.UpdateSpatial();

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

	m_World.PostUpdate();

	if (m_pSkybox) {
		m_pSkybox->Update();
	}

	// Update UI Components
	if (m_pUIBoard) {
		m_pUIBoard->Update();
	}
}

void Scene::PrepareRender()
{
	/* Old xz grid space partition */
	//if (m_pPlayer)
	//	m_pPlayer->Render();
	//
	//for (auto& pObj : m_SpacePartition.ObjectBroadPhaseFrustumCulling(GetCamera()->GetFrustumWorld())) {
	//	pObj->Render();
	//}

	m_World.UpdateSpatial();

	SpatialQueryDesc desc{};
	desc.unLayerMask = SPATIAL_RENDERABLE;
	desc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
	desc.bIncludeStatic = true;
	desc.bIncludeDynamic = true;


	SpatialQueryResult visibleGrid = m_World.GetSpatial().QueryFrustum(
		GetCamera()->GetFrustumWorld(),
		desc
	);

	ImGui::Text("Spatial Grid Query Result : %d", visibleGrid.pObjects.size());
	m_World.GetSpatial().DebugPrintProxyStats();

	if (m_pPlayer) {
		m_pPlayer->Render();
	}

	for (auto* pObj : visibleGrid.pObjects) {
		if (pObj) {
			pObj->Render();
		}
	}

	//m_World.PrepareRender<NetworkRemoteThirdPersonPlayer>();
	//m_World.PrepareRender<Zombie>();
	//m_World.PrepareRender<WeaponObject>();
}

void Scene::CheckCollision()
{
	// Broad Phase
	if (!m_pPlayer) {
		return;
	}

	const std::shared_ptr<PlayerCollider> playerCollider = m_pPlayer->GetComponent<PlayerCollider>();
	if (!playerCollider) {
		return;
	}

	SpatialQueryDesc staticCollisionDesc{};
	staticCollisionDesc.unLayerMask =
		SPATIAL_COLLIDABLE |
		SPATIAL_STATIC;

	staticCollisionDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
	staticCollisionDesc.bIncludeStatic = true;
	staticCollisionDesc.bIncludeDynamic = false;
	staticCollisionDesc.v3AABBInflation = Vector3{ 5.f, 20.f, 5.f };
	SpatialQueryResult playerBroadPhaseResult = m_World.GetSpatial().QueryAABB(playerCollider->GetAABBFromOBBWorld(), staticCollisionDesc);
	ImGui::Text("Player Collision Candidates : %d", (int)playerBroadPhaseResult.pObjects.size());

	// Player vs StaticObject
	for (const auto& pObj : playerBroadPhaseResult.pObjects) {
		const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>(); 
		if (!pCollider) {
			continue;
		}

		bool bResult = playerCollider->CheckCollision(pCollider);
		if (bResult) {
			CollisionResult result1(m_pPlayer.get(), pObj);
			CollisionResult result2(pObj, m_pPlayer.get());
			if (!m_pCollisionPairs.contains(result1) || !m_pCollisionPairs.contains(result2)) {
				// Begin Overlap
				m_pPlayer->OnBeginCollision(result1);
				pObj->OnBeginCollision(result2);

				m_pCollisionPairs.insert(result1);
				m_pCollisionPairs.insert(result2);
			}
			else {
				// While Overlap
				m_pPlayer->OnWhileCollision(CollisionResult(m_pPlayer.get(), pObj));
				pObj->OnWhileCollision(CollisionResult(pObj, m_pPlayer.get()));
			}
		}
		else {
			// End Overlap
			CollisionResult result1(m_pPlayer.get(), pObj);
			CollisionResult result2(pObj, m_pPlayer.get());
			if (m_pCollisionPairs.contains(result1) || m_pCollisionPairs.contains(result2)) {
				m_pPlayer->OnEndCollision(result1);
				pObj->OnEndCollision(result2);

				m_pCollisionPairs.erase(result1);
				m_pCollisionPairs.erase(result2);
			}
		}
	}

	// ── 좀비 vs 정적 오브젝트 ──────────────────────────────────────────────
	for (auto& pZombie : m_World.GetObjects<Zombie>()) {
		auto pZombieCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pZombieCollider)
			continue;

		SpatialQueryResult zombieBroadPhaseResult = m_World.GetSpatial().QueryAABB(pZombieCollider->GetAABBFromOBBWorld(), staticCollisionDesc);
		const PlayerCollider& zombieCollider = *pZombieCollider;

		for (const auto& pObj : zombieBroadPhaseResult.pObjects) {
			const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>();
			if (!pCollider)
				continue;

			bool bResult = pCollider->CheckCollision(pZombieCollider);
			if (bResult) {
				CollisionResult result1(pZombie.get(), pObj);
				CollisionResult result2(pObj, pZombie.get());
				if (!m_pCollisionPairs.contains(result1) || !m_pCollisionPairs.contains(result2)) {
					pZombie->OnBeginCollision(result1);
					pObj->OnBeginCollision(result2);
					m_pCollisionPairs.insert(result1);
					m_pCollisionPairs.insert(result2);
				}
				else {
					pZombie->OnWhileCollision(CollisionResult(pZombie.get(), pObj));
					pObj->OnWhileCollision(CollisionResult(pObj, pZombie.get()));
				}
			}
			else {
				CollisionResult result1(pZombie.get(), pObj);
				CollisionResult result2(pObj, pZombie.get());
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
	for (const auto& pObj : m_World.GetObjects<StaticObject>()) {
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

void Scene::RemoveCollisionPairsOf(IGameObject* pDeadObject)
{
	if (!pDeadObject) {
		return;
	}

	std::erase_if(m_pCollisionPairs,
		[pDeadObject](const CollisionResult& result)
		{
			return result.pSelf == pDeadObject ||
				result.pOther == pDeadObject;
		});
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
	std::string strFilePath = std::format("{}/{}.bin", g_strSceneBasePath, strFileName);

	std::ifstream inFile{ strFilePath, std::ios::binary };
	if (!inFile) {
		__debugbreak();
		return E_INVALIDARG;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	nlohmann::json jScene = nlohmann::json::from_bson(bson);;

	if (jScene.contains("StaticMeshActors")) {
		size_t nObjects = jScene["StaticMeshActors"].size();
		m_World.Reserve<StaticObject>(nObjects);
		for (const auto& jObject : jScene["StaticMeshActors"]) {
			std::shared_ptr<StaticObject> pObj = std::make_shared<StaticObject>();
			pObj->SetName(jObject["ActorName"].get<std::string>());

			auto matrixData = jObject["Transform"]["WorldMatrix"].get<std::vector<float>>();
			Matrix mtxWorldMatrix(matrixData.data());
			pObj->GetTransform()->SetWorldMatrix(mtxWorldMatrix);

			std::string strMeshName = jObject["MeshName"].get<std::string>();
			auto pMeshObject = MODEL->LoadOrGet(strMeshName, true)->CopyObject<NodeObject>();
			pObj->SetChild(pMeshObject);

			// 콜리전 메시를 자식이 아닌 루트 StaticObject에 직접 추가
			const auto* pCollisionInfos = MODEL->GetCollisionInfos(strMeshName);
			if (pCollisionInfos) {
				for (const auto& info : *pCollisionInfos) {
					pObj->AddComponent<MeshCollider>(info);
				}
			}

			//m_pGameObjects.push_back(pObj);
			AddObject(pObj);
		}
	}

	// Lights 로드

	if (jScene.contains("Lights")) {
		size_t nLights = jScene["Lights"].size();
		m_pLights.reserve(nLights);
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
