#include "pch.h"
#include "Scene.h"
#include "ThirdPersonPlayer.h"
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
	// Call after async BuildObjects()
	FinalizeBuild();

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

	//if (m_pPlayer) {
	//	m_pPlayer->GetTransform()->SetPosition(m_xmSceneBound.Center);
	//}

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
	// 컷씬 중 게임플레이 정지는 이 입력 차단 하나로만 한다 — 나머지 업데이트
	// (물리/카메라/애니/월드)는 계속 돌아 캐릭터가 자연스럽게 멈춰 Idle로 전환됨
	if (IsCinematicActive()) {
		// 입력을 스킵하면 마지막 프레임의 이동 플래그가 남아 계속 달린다 —
		// 잔상을 지워 마찰 감속으로 자연 정지시킨다
		if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			pThirdPerson->ClearMovementInput();
		}
		return;
	}

	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
	}

	m_World.PostProcessInput();
}

void Scene::PushCinematic()
{
	// 첫 진입(0→1): 조준 중이면 해제 — 컷씬 화면에 크로스헤어/줌이 남지 않게
	if (m_nCinematicDepth == 0 && m_pPlayer) {
		if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			pThirdPerson->LeaveAim();
		}
	}
	++m_nCinematicDepth;
}

void Scene::PreUpdate()
{
	// 컷씬 중에도 월드는 계속 구동 — 정지는 입력 차단(PostProcessInput)만으로 한다.
	// 하드 프리즈하면 애니메이션이 포즈 그대로 멈추고, 리모트 스냅샷이 쌓였다가
	// 컷씬 종료 시 위치가 튄다. 좀비는 서버(온라인)/Zombie::PostUpdate(오프라인)가
	// 이동·AI만 홀드해 제자리에서 Idle로 자연 전환된다.
	if (m_pPlayer) {
		m_pPlayer->PreUpdate();
	}

	m_World.PreUpdate();
}

void Scene::FixedUpdate()
{
	// Component Update — 컷씬 중에도 정지하지 않음 (입력만 차단, PreUpdate 주석 참고)
	if (m_pPlayer) {
		m_pPlayer->Update();
	}

	if (m_pTerrain) {
		m_pTerrain->Update();
	}

	m_World.FixedUpdate();
	m_World.UpdateSpatial();

	m_ToneMappingVolume.Update();

	// 컷씬 이벤트는 계속 구동되어야 연출(카메라/시간/포스트FX)이 진행됨.
	if (m_pEventSequence) {
		m_pEventSequence->Update();
	}

	CheckCollision();
}

void Scene::PostUpdate()
{
	// Post Update — 컷씬 중에도 정지하지 않음 (입력만 차단, PreUpdate 주석 참고)
	if (m_pPlayer) {
		m_pPlayer->PostUpdate();
	}

	if (m_pTerrain) {
		m_pTerrain->PostUpdate();
	}

	m_World.PostUpdate();

	if (!SCENE->IsInAsyncSceneChanging()) {
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
		//const auto& pTransform = m_pPlayer->GetTransform();
		//auto v3Pos = pTransform->GetPosition();
		//v3Pos.y += 1.75_m;
		//SOUND->SetListenerAttributes(v3Pos, Vector3::Zero, pTransform->GetLook(), pTransform->GetUp());
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

	if (m_pPlayer && !m_bHideCharacters) {
		m_pPlayer->Render();
	}

	// 컷씬 임시 오브젝트(스폐셜 미등록) 직접 렌더
	if (m_pCinematicProp) {
		m_pCinematicProp->Render();
	}

	for (auto* pObj : visibleGrid.pObjects) {
		if (!pObj) continue;
		if (m_bHideCharacters && pObj->IsCharacter()) continue;  // 탈출 컷씬: 전 플레이어 숨김
		pObj->Render();
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
	if (pNewCamera) {
		pNewCamera->Resize(
			WinCore::g_dwClientWidth,
			WinCore::g_dwClientHeight
		);
	}

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

// 구조 헬기(착륙) 경로점을 별도 JSON에서 로드 (추락 경로 LoadHeliPath 와 동일 패턴, 키만 "ArrivePath").
// 언리얼 SaveHeliArrivePathToJson 출력 → { "ArrivePath": [ { Transform.WorldMatrix } ] }
HRESULT Scene::LoadHeliArrivePath(const std::string& strFileName)
{
	m_v3HeliArrivePath.clear();

	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);
	std::ifstream in(strFilePath);
	if (!in) {
		OutputDebugStringA(std::format("[HeliArrivePath] FAIL open: {}\n", strFilePath).c_str());
		return E_INVALIDARG; // 경로 파일 없음 → 직선 폴백
	}

	nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
	if (j.is_discarded() || !j.contains("ArrivePath")) {
		OutputDebugStringA(std::format("[HeliArrivePath] FAIL parse/key: {}\n", strFilePath).c_str());
		return E_FAIL;
	}

	m_v3HeliArrivePath.reserve(j["ArrivePath"].size());
	for (const auto& jPoint : j["ArrivePath"]) {
		Matrix mtxWorld = ::ReadMatrixFromJson(jPoint["Transform"]["WorldMatrix"]);
		m_v3HeliArrivePath.emplace_back(mtxWorld._41, mtxWorld._42, mtxWorld._43);
	}

	OutputDebugStringA(std::format("[HeliArrivePath] LOADED {} points from {}\n",
		m_v3HeliArrivePath.size(), strFilePath).c_str());
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


//////////////////////////////////////////////////////////////////////////////////////////////
// Scene Loading

struct STATICOBJECTLOADDESC {
	std::string strObjectName;
	std::string strMeshName;
	Matrix mtxWorld;

	size_t unModelIndex = 0;
	std::shared_ptr<IGameObject> pModel;

	static std::vector<STATICOBJECTLOADDESC> ParseStaticObjectFromJson(const nlohmann::json& jScene) {
		std::vector<STATICOBJECTLOADDESC> loadDescs;

		if (!jScene.contains("StaticMeshActors")) {
			return loadDescs;
		}

		const auto& jObjects = jScene["StaticMeshActors"];
		size_t nObjects = jObjects.size();
		loadDescs.reserve(nObjects);
		for (const auto& jObject : jObjects) {
			loadDescs.push_back(STATICOBJECTLOADDESC{
				.strObjectName = jObject["ActorName"].get<std::string>(),
				.strMeshName = jObject["MeshName"].get<std::string>(),
				.mtxWorld = ::ReadMatrixFromJson(jObject["Transform"]["WorldMatrix"])
				}
			);
		}
		
		return loadDescs;
	}

};

HRESULT Scene::LoadFromFiles(const std::string& strFileName)
{
	std::string strFilePath = std::format("{}/{}.bin", g_strSceneBasePath, strFileName);

	auto bson = ::ReadBinaryFile(strFilePath);
	if (bson.empty()) {
		__debugbreak();
		return E_INVALIDARG;
	}

	nlohmann::json jScene = nlohmann::json::from_bson(bson);;

#ifndef TIME_RECORD
	auto staticObjectLoadDesc = STATICOBJECTLOADDESC::ParseStaticObjectFromJson(jScene);
	m_World.Reserve<StaticObject>(staticObjectLoadDesc.size());

	std::unordered_map<std::string, size_t> modelMap;
	modelMap.reserve(staticObjectLoadDesc.size());

	std::vector<std::string> strModelNames;
	strModelNames.reserve(staticObjectLoadDesc.size());

	for (auto& desc : staticObjectLoadDesc) {
		auto [it, bInserted] = modelMap.try_emplace(desc.strMeshName, strModelNames.size());
		if (bInserted) {
			strModelNames.push_back(desc.strMeshName);
		}

		desc.unModelIndex = it->second;
	}

	std::vector<std::shared_ptr<IGameObject>> pLoadedModels(strModelNames.size());
	tbb::parallel_for(tbb::blocked_range<size_t>(0, strModelNames.size()), [&](const tbb::blocked_range<size_t>& range) {
		for (size_t i = range.begin(); i < range.end(); ++i) {
			pLoadedModels[i] = MODEL->LoadOrGet(strModelNames[i], true);
		}
		});

	for (auto& desc : staticObjectLoadDesc) {
		desc.pModel = pLoadedModels[desc.unModelIndex];

		if (!desc.pModel) {
			assert(false && "Model Loading Failed");
			return E_FAIL;
		}
	}

	if (staticObjectLoadDesc.size() < 30) {
		for (const auto& desc : staticObjectLoadDesc) {
			auto pObj = std::make_shared<StaticObject>();
			pObj->SetName(desc.strObjectName);
			pObj->GetTransform()->SetWorldMatrix(desc.mtxWorld);

			auto pMeshObject = desc.pModel->CopyObject<NodeObject>();
			pObj->SetChild(pMeshObject);

			AddObject(pObj);
		}
	}
	else {
		std::vector<std::shared_ptr<StaticObject>> pLoadedObjects(staticObjectLoadDesc.size());
		tbb::parallel_for(tbb::blocked_range<size_t>(0, staticObjectLoadDesc.size()), [&](const tbb::blocked_range<size_t>& range) {
			for (size_t i = range.begin(); i < range.end(); ++i) {
				const auto& desc = staticObjectLoadDesc[i];
				auto pObj = std::make_shared<StaticObject>();

				pObj->SetName(desc.strObjectName);
				pObj->GetTransform()->SetWorldMatrix(desc.mtxWorld);

				auto pMeshObject = desc.pModel->CopyObject<NodeObject>();
				pObj->SetChild(pMeshObject);

				pLoadedObjects[i] = std::move(pObj);
			}
			});

		for (const auto& pObj : pLoadedObjects) {
			AddObject(pObj);
		}
	}
#else
	using time = std::chrono::high_resolution_clock;

	decltype(time::now()) beginLoad, endLoad, beginParse, endParse, beginModel, endModel, beginClone, endClone, beginMerge, endMerge;


	beginLoad = time::now();

	beginParse = time::now();
	std::vector<STATICOBJECTLOADDESC> staticObjectLoadDesc;
	{
		staticObjectLoadDesc = STATICOBJECTLOADDESC::ParseStaticObjectFromJson(jScene);
		m_World.Reserve<StaticObject>(staticObjectLoadDesc.size());
	}
	endParse = time::now();

	beginModel = time::now();
	size_t unModelCount = 0;
	std::unordered_map<std::string, size_t> modelMap;
	{
		modelMap.reserve(staticObjectLoadDesc.size());

		std::vector<std::string> strModelNames;
		strModelNames.reserve(staticObjectLoadDesc.size());

		// Gather unique model name and generate object model index
		for (auto& desc : staticObjectLoadDesc) {
			auto [it, bInserted] = modelMap.try_emplace(desc.strMeshName, strModelNames.size());
			if (bInserted) {
				strModelNames.push_back(desc.strMeshName);
			}
			desc.unModelIndex = it->second;
		}
		unModelCount = strModelNames.size();

		// Model parallel loading
		std::vector<std::shared_ptr<IGameObject>> pLoadedModels(unModelCount);
		tbb::parallel_for(tbb::blocked_range<size_t>(0, strModelNames.size()), [&](const tbb::blocked_range<size_t>& range) {
			for (size_t i = range.begin(); i < range.end(); ++i) {
				pLoadedModels[i] = MODEL->LoadOrGet(strModelNames[i], true);
			}
		});

		// Link object - model after parallel loading
		for (auto& desc : staticObjectLoadDesc) {
			desc.pModel = pLoadedModels[desc.unModelIndex];

			if (!desc.pModel) {
				assert(false && "Model Loading Failed");
				return E_FAIL;
			}
		}

	}
	endModel = time::now();


	std::vector<std::shared_ptr<StaticObject>> pLoadedObjects(staticObjectLoadDesc.size());
	auto fnLoadObjects = [&](size_t i) {
		const auto& desc = staticObjectLoadDesc[i];

		auto pObj = std::make_shared<StaticObject>();
		pObj->SetName(desc.strObjectName);
		pObj->GetTransform()->SetWorldMatrix(desc.mtxWorld);

		auto pMeshObject =
			desc.pModel->CopyObject<NodeObject>();

		pObj->SetChild(pMeshObject);
		pLoadedObjects[i] = std::move(pObj);
	};

	beginClone = time::now();
	{
		//for (size_t i = 0; i < staticObjectLoadDesc.size(); ++i) {
		//	fnLoadObjects(i);
		//}

		if (staticObjectLoadDesc.size() < 30) {
			for (size_t i = 0; i < staticObjectLoadDesc.size(); ++i) {
				fnLoadObjects(i);
			}
		}
		else {
			tbb::parallel_for(tbb::blocked_range<size_t>(0, staticObjectLoadDesc.size()), [&](const tbb::blocked_range<size_t>& range) {
				for (size_t i = range.begin(); i < range.end(); ++i) {
					fnLoadObjects(i);
				}
				});
		}
	}
	endClone = time::now();

	beginMerge = time::now();
	{
		for (const auto& pObj : pLoadedObjects) {
			AddObject(pObj);
		}
	}
	endMerge = time::now();

	endLoad = time::now();

	auto fnTpToMs = [](auto beg, auto end) -> long long {
		return std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count();
	};

	OutputDebugStringA(
		std::format(
			"[SceneLoad] objects={} models={} "
			"parse={}ms preload={}ms clone={}ms "
			"merge={}ms total={}ms\n",
			staticObjectLoadDesc.size(),
			modelMap.size(),
			fnTpToMs(beginParse, endParse),
			fnTpToMs(beginModel, endModel),
			fnTpToMs(beginClone, endClone),
			fnTpToMs(beginMerge, endMerge),
			fnTpToMs(beginLoad, endLoad)
		).c_str()
	);

#endif






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

