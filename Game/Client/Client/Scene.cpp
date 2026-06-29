#include "pch.h"
#include "Scene.h"
#include "TerrainObject.h"
#include "NodeObject.h"
#include "Collider.h"
#include "Skybox.h"
#include "EventSequence.h"

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
	if (m_pPlayer) {
		m_pMainCamera = m_pPlayer->GetCamera();
	}

	GenerateSceneBound();

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
				false
			);
		}

		// 2. Update Proxy bounds
		m_World.UpdateSpatial();
		m_World.GetSpatial().BuildStaticGrid(gridDesc);
	}

	if (m_pPlayer) {
		//m_pPlayer->GetTransform()->SetPosition(m_xmSceneBound.Center);
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

	if (!m_pUIBoard) {
		m_pUIBoard = std::make_unique<UIBoard>();
	}

	if (m_pEventSequence) {
		m_pEventSequence->Initialize();
	}

}

void Scene::PreProcessInput()
{
	// TODO : Cache last frame world transform
	// Reason : to generate motion vector
}

void Scene::PostProcessInput()
{
	if (IsCinematicActive()) return; // 컷씬 중 플레이어 입력 차단

	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
	}

	m_World.PostProcessInput();
}

void Scene::PreUpdate()
{
	if (IsCinematicActive()) return; // 컷씬 중 정지

	if (m_pPlayer) {
		m_pPlayer->PreUpdate();
	}

	m_World.PreUpdate();
}

void Scene::FixedUpdate()
{
	const bool bFrozen = IsCinematicActive(); // 컷씬 중 플레이어/월드(좀비) 정지

	// Component Update
	if (!bFrozen) {
		if (m_pPlayer) {
			m_pPlayer->Update();
		}

		if (m_pTerrain) {
			m_pTerrain->Update();
		}

		m_World.FixedUpdate();
		m_World.UpdateSpatial();
	}

	m_ToneMappingVolume.Update();

	// 컷씬 이벤트는 계속 구동되어야 연출(카메라/시간/포스트FX)이 진행됨.
	if (m_pEventSequence) {
		m_pEventSequence->Update();
	}

	if (!bFrozen) {
		CheckCollision();
	}
}

void Scene::PostUpdate()
{
	const bool bFrozen = IsCinematicActive(); // 컷씬 중 플레이어/월드(좀비) 이동·애니 정지

	// Post Update
	if (!bFrozen) {
		if (m_pPlayer) {
			m_pPlayer->PostUpdate();
		}

		if (m_pTerrain) {
			m_pTerrain->PostUpdate();
		}

		m_World.PostUpdate();

		ANIMATION->UpdateAnimationParallel();

		if (m_pPlayer) {
			m_pPlayer->PostAnimationUpdate();
		}
		m_World.PostAnimationUpdate();
	}

	// 스카이박스(시간/태양)는 컷씬 중에도 갱신 — 석양 연출이 진행되어야 함.
	if (m_pSkybox) {
		m_pSkybox->Update();
	}

	// Update UI Components
	if (m_pUIBoard) {
		m_pUIBoard->Update();
	}

	// Update 3D audio listener from the active camera.
	if (m_pPlayer) {
		const std::shared_ptr<Camera>& pCamera = GetCamera();
		SOUND->SetListenerAttributes(pCamera->GetPosition(), Vector3::Zero, pCamera->GetLook(), pCamera->GetUp());
	}
}

void Scene::PrepareRender()
{
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

	if (m_pPlayer) {
		m_pPlayer->Render();
	}

	// 컷씬 임시 오브젝트(스폐셜 미등록) 직접 렌더
	if (m_pCinematicProp) {
		m_pCinematicProp->Render();
	}

	for (auto* pObj : visibleGrid.pObjects) {
		if (pObj) {
			pObj->Render();
		}
	}
}

void Scene::RemoveInvalidCollisionSet(const SpatialQueryResult& playerBroadPhaseResult)
{
	const auto& broadPhase = playerBroadPhaseResult.pObjects;
	std::unordered_set<IGameObject*> validset{ broadPhase.begin(), broadPhase.end() };
	for (auto it = m_pCollisionPairs.begin(); it != m_pCollisionPairs.end(); ++it) {
		if (!validset.contains(it->pSelf)) {
			it = m_pCollisionPairs.erase(it);
		}
		else if (!validset.contains(it->pOther)) {
			it = m_pCollisionPairs.erase(it);
		}
		else {
			++it;
		}

	}
}

void Scene::CheckCollision()
{
	// Broad Phase
	if (!m_pPlayer) {
		return;
	}

	SpatialQueryDesc staticCollisionDesc{};
	staticCollisionDesc.unLayerMask =
		SPATIAL_COLLIDABLE |
		SPATIAL_STATIC;

	staticCollisionDesc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
	staticCollisionDesc.bIncludeStatic = true;
	staticCollisionDesc.bIncludeDynamic = false;
	staticCollisionDesc.v3AABBInflation = Vector3{ 3_m, 3_m, 3_m };

	const std::shared_ptr<PlayerCollider> playerCollider = m_pPlayer->GetComponent<PlayerCollider>();
	if (playerCollider) {
		SpatialQueryResult playerBroadPhaseResult = m_World.GetSpatial().QueryAABB(playerCollider->GetAABBFromOBBWorld(), staticCollisionDesc);
		//ImGui::Text("Player Collision Candidates : %d", (int)playerBroadPhaseResult.pObjects.size());
		//RemoveInvalidCollisionSet(playerBroadPhaseResult);

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
	}

	// ── 좀비 vs 정적 오브젝트 ──────────────────────────────────────────────
	for (auto& pZombie : m_World.GetObjects<Zombie>()) {
		auto pZombieCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pZombieCollider)
			continue;

		SpatialQueryResult zombieBroadPhaseResult = m_World.GetSpatial().QueryAABB(pZombieCollider->GetAABBFromOBBWorld(), staticCollisionDesc);
		const PlayerCollider& zombieCollider = *pZombieCollider;
		//RemoveInvalidCollisionSet(zombieBroadPhaseResult);

		for (const auto& pObj : zombieBroadPhaseResult.pObjects) {
			const std::shared_ptr<StaticCollider> pCollider = pObj->GetComponent<StaticCollider>();
			if (!pCollider)
				continue;

			bool bResult = zombieCollider.CheckCollision(pCollider);
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

std::shared_ptr<Camera> Scene::SwapCamera(std::shared_ptr<Camera>& pNewCamera)
{
	std::shared_ptr<Camera> pBefore = m_pMainCamera;
	m_pMainCamera = pNewCamera;
	return pBefore;
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
	m_pLights.reserve(m_pLights.size() + 1);

	auto pLight = std::make_shared<DirectionalLight>();
	{
		pLight->m_v3Color = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Direction = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Position = Vector3{ 100.f, 10000.f, 100.f };
		pLight->m_fIntensity = 0.2;

		pLight->m_v3Direction.Normalize();
	}

	m_pLights.insert(m_pLights.begin(), pLight);
}

std::shared_ptr<DirectionalLight> Scene::GetSunLight() const
{
	/*for (const auto& pLight : m_pLights) {
		if (auto pDirectionalLight = std::dynamic_pointer_cast<DirectionalLight>(pLight)) {
			return pDirectionalLight;
		}
	}*/

	if (m_pSkybox) {
		return std::static_pointer_cast<DirectionalLight>(m_pLights[0]);
	}

	return nullptr;
}

HRESULT Scene::LoadFromFiles(const std::string& strFileName)
{
	std::string strFilePath = std::format("{}/{}.bin", g_strSceneBasePath, strFileName);

	auto bson = ::ReadBinaryFile(strFilePath);
	if (bson.empty()) {
		__debugbreak();
		return E_INVALIDARG;
	}

	nlohmann::json jScene = nlohmann::json::from_bson(bson);;

	if (jScene.contains("StaticMeshActors")) {
		size_t nObjects = jScene["StaticMeshActors"].size();
		m_World.Reserve<StaticObject>(nObjects);
		for (const auto& jObject : jScene["StaticMeshActors"]) {
			std::shared_ptr<StaticObject> pObj = std::make_shared<StaticObject>();
			pObj->SetName(jObject["ActorName"].get<std::string>());

			Matrix mtxWorldMatrix = ::ReadMatrixFromJson(jObject["Transform"]["WorldMatrix"]);
			pObj->GetTransform()->SetWorldMatrix(mtxWorldMatrix);

			std::string strMeshName = jObject["MeshName"].get<std::string>();
			auto pMeshObject = MODEL->LoadOrGet(strMeshName, true)->CopyObject<NodeObject>();
			pObj->SetChild(pMeshObject);

			// 콜리전 메시를 자식이 아닌 루트 StaticObject에 직접 추가
			//const auto* pCollisionInfos = MODEL->GetCollisionInfos(strMeshName);
			//if (pCollisionInfos) {
			//	for (const auto& info : *pCollisionInfos) {
			//		pObj->AddComponent<StaticCollider>(info);
			//	}
			//}

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
				Matrix mtxWorldMatrix = ::ReadMatrixFromJson(jLight["Transform"]["WorldMatrix"]);
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
				Matrix mtxWorldMatrix = ::ReadMatrixFromJson(jLight["Transform"]["WorldMatrix"]);
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

	return S_OK;
}

// 좀비 스폰 포인트를 별도 JSON 파일에서 로드 (씬 .bin 과 분리).
// 언리얼 SaveSpawnPointsToJson 출력 → { "ZombieSpawnPoints": [ { Transform.WorldMatrix } ] }
// WorldMatrix translation(_41,_42,_43)에서 위치만 추출. 파일 없으면 빈 목록.
HRESULT Scene::LoadZombieSpawnPoints(const std::string& strFileName)
{
	m_v3ZombieSpawnPoints.clear();

	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);
	std::ifstream in(strFilePath);
	if (!in)
		return E_INVALIDARG; // 스폰 포인트 파일 없음 → 스폰 안 함

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("ZombieSpawnPoints"))
		return E_FAIL;

	m_v3ZombieSpawnPoints.reserve(j["ZombieSpawnPoints"].size());
	for (const auto& jSpawn : j["ZombieSpawnPoints"]) {
		Matrix mtxWorld = ::ReadMatrixFromJson(jSpawn["Transform"]["WorldMatrix"]);
		m_v3ZombieSpawnPoints.emplace_back(mtxWorld._41, mtxWorld._42, mtxWorld._43);
	}

	return S_OK;
}

// 헬기 비행 경로점을 별도 JSON에서 로드 (씬 .bin 과 분리).
// 언리얼 SaveHeliPathToJson 출력 → { "HeliPath": [ { Transform.WorldMatrix } ] }
// 익스포터가 이름 접두사(HeliPath_N) 숫자 순으로 정렬해 내보내므로 순서를 그대로 보존.
// WorldMatrix translation(_41,_42,_43)에서 위치만 추출. 파일 없으면 빈 목록.
HRESULT Scene::LoadHeliPath(const std::string& strFileName)
{
	m_v3HeliPath.clear();

	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);
	std::ifstream in(strFilePath);
	if (!in) {
		OutputDebugStringA(std::format("[HeliPath] FAIL open: {}\n", strFilePath).c_str());
		return E_INVALIDARG; // 경로 파일 없음 → 직선 폴백
	}

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("HeliPath")) {
		OutputDebugStringA(std::format("[HeliPath] FAIL parse/key: {}\n", strFilePath).c_str());
		return E_FAIL;
	}

	m_v3HeliPath.reserve(j["HeliPath"].size());
	for (const auto& jPoint : j["HeliPath"]) {
		Matrix mtxWorld = ::ReadMatrixFromJson(jPoint["Transform"]["WorldMatrix"]);
		m_v3HeliPath.emplace_back(mtxWorld._41, mtxWorld._42, mtxWorld._43);
	}

	OutputDebugStringA(std::format("[HeliPath] LOADED {} points from {}\n",
		m_v3HeliPath.size(), strFilePath).c_str());
	return S_OK;
}

void Scene::ShowDebugOptions()
{
	if (ImGui::BeginTabBar("Scene Options")) {
		if(ImGui::BeginTabItem("World")){
			m_World.GetSpatial().DebugPrintProxyStats();
			ImGui::EndTabItem();		
		}
		if(ImGui::BeginTabItem("Tone Mapping Volume")){
			m_ToneMappingVolume.ShowDebugOptions();
			ImGui::EndTabItem();		
		}
		if(ImGui::BeginTabItem("Post Processing Volume")){
			m_PostProcessingVolume.ShowDebugOptions();
			ImGui::EndTabItem();		
		}
		if(ImGui::BeginTabItem("Lights")){
			ImGui::Text("Elapsed TIme : %f", TIME->GetTimeElapsed());
			ImGui::Text("Total TIme : %f", TIME->GetTotalTime());

			float fAmbient = m_v4GlobalAmbient.x;
			ImGui::DragFloat("GlobalAmbient", (float*)&fAmbient, 0.001f, 0.f, 1.f);
			m_v4GlobalAmbient = XMVectorReplicate(fAmbient);
			ImGui::Text("NumLights : %d", m_pLights.size());
			for (uint32 i = 0; i < m_pLights.size(); ++i) {
				if (ImGui::TreeNode(std::format("Index : {}", i).c_str())) {
					m_pLights[i]->ShowControllImGui();
					ImGui::TreePop();
				}
			}
			ImGui::EndTabItem();		
		}
		if(ImGui::BeginTabItem("Player")){
			if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
				m_pPlayer->ShowControlImGui();

				ImGui::Text("====== Weapon Test ======");
				const auto eWeaponType = pPlayer->GetCurrentWeaponType();
				auto nWeaponIdx = std::to_underlying(eWeaponType);
				const auto& strWeaponNames = GameContext::g_strWeaponNames;
				if (ImGui::BeginCombo("Weapons", strWeaponNames[nWeaponIdx].c_str())) {
					for (int i = 0; i < strWeaponNames.size(); ++i) {
						bool bSelected = (nWeaponIdx == i);
						if (ImGui::Selectable(strWeaponNames[i].c_str(), bSelected)) {
							nWeaponIdx = i;
							if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
								pThirdPerson->GiveWeapon(static_cast<WEAPON_TYPE>(nWeaponIdx));
							}
						}

						if (bSelected) {
							ImGui::SetItemDefaultFocus();
						}

					}

					ImGui::EndCombo();
				}

				if (ImGui::TreeNode("Weapon Options")) {
					pPlayer->GetCurrentWeaponObject()->ShowControlImGui();
					ImGui::TreePop();
				}

				ImGui::Text("Press `(~) to use mouse control");
				ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");

				ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());

				const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
				ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);

				const auto& transform = pPlayer->GetTransform();
				Vector3 v3PlayerPos = transform->GetPosition();
				ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

				ImGui::Text("====== Collision Result ======");
				for (const auto& pair : m_pCollisionPairs) {
					ImGui::Text("Collision {%s : %s}", pair.pSelf->GetName().c_str(), pair.pOther->GetName().c_str());
				}
			}
			ImGui::EndTabItem();		
		}

		if(ImGui::BeginTabItem("Terrain")){
			ImGui::Text("No Options");
			ImGui::EndTabItem();		
		}

		if(ImGui::BeginTabItem("Skybox")){
			if (m_pSkybox) {
				m_pSkybox->ShowControllImGui();
			}
			else {
				ImGui::Text("NULL Skybox");
			}
			ImGui::EndTabItem();		
		}

		if(ImGui::BeginTabItem("UI")){
			if (m_pUIBoard) {
				m_pUIBoard->EditUI();
			}
			else {
				ImGui::Text("NULL UI");
			}
			ImGui::EndTabItem();		
		}

		ImGui::EndTabBar();
	}

	
}

