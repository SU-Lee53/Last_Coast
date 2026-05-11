#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "Zombie.h"
#include "ZombieAnimationController.h"
#include "Skybox.h"
#include "ThirdPersonCamera.h"
#include "Collider.h"
#include "MapTestScene.h"

void TestScene::BuildObjects()
{
	m_pUIBoard = std::make_unique<UIBoard>();

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) {
		m_pPlayer = std::make_shared<LocalThirdPersonPlayer>();
	}
	else {
		m_pPlayer = std::make_shared<NetworkOwnerThirdPersonPlayer>();
	}
	m_pPlayer->Initialize();

	//for (int i = 0; i < 100; ++i) {
	//	std::shared_ptr<Zombie> pObj = std::make_shared<Zombie>();
	//	//m_pGameObjects.push_back(pObj);
	//	AddObject<Zombie>(pObj);
	//}

	if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		pThirdPerson->GiveWeapon(WEAPON_TYPE::M4);
	}
	
	// 서버 연결 시: 모든 좀비를 dormant로 시작하고 서버 스폰 명령을 기다린다.
	// 오프라인 시: 기존처럼 active로 시작해 로컬 AI가 동작한다.
	bool bOnline = NETWORK->IsConnected() && !NETWORK->IsOffline();
	for (auto& pZombie : m_ZombiePool.Initialize(100, bOnline))
		AddObject(pZombie);

	LoadFromFiles("TEST1");

	//Scene::InitializeObjects();

	// NavMesh 디버그 렌더러 초기화
	//m_pNavMeshDebugRenderer = std::make_unique<NavMeshDebugRenderer>();
	//if (AI->IsInitialized()) {
	//	m_pNavMeshDebugRenderer->Initialize(AI->GetNavMesh());
	//	// RenderManager에 등록
	//	RENDER->SetNavMeshDebugRenderer(m_pNavMeshDebugRenderer.get());
	//}
}

void TestScene::OnEnterScene()
{
}

void TestScene::OnLeaveScene()
{
}

void TestScene::ProcessInput()
{
}

void TestScene::Update()
{
	ImGui::Begin("Test");
	{
		if (ImGui::Button("Change Scene")) {
			SCENE->ChangeScene<MapTestScene>();
			ImGui::End();
			return;
		}

		if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			ImGui::Text("Press `(~) to use mouse control");
			ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");
			ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());
			const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
			ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);
			const auto& transform = pPlayer->GetTransform();
			Vector3 v3PlayerPos = transform->GetPosition();
			ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

			// ★ 플레이어 체력
			ImGui::NewLine();
			ImGui::Text("====== Player HP ======");
			float fHP = pPlayer->GetHP();
			float fMaxHP = pPlayer->GetMaxHP();
			ImGui::Text("HP : %.0f / %.0f", fHP, fMaxHP);
			ImGui::ProgressBar(fHP / fMaxHP, ImVec2(-1.f, 0.f));
			if (pPlayer->IsDead()) ImGui::TextColored(ImVec4(1, 0, 0, 1), "DEAD");

			// ★ 좀비 위치 출력 (서버 제어 좀비만)
			ImGui::NewLine();
			bool bOnlineMode = NETWORK->IsConnected() && !NETWORK->IsOffline();
			ImGui::Text("====== Zombie [%s] (%d) ======",
				bOnlineMode ? "Server" : "Local",
				bOnlineMode ? (int)m_ServerZombies.size() : (int)m_World.GetObjects<Zombie>().Size());
			int zombieIdx = 0;
			auto ShowZombieDebug = [&](const std::shared_ptr<Zombie>& pZombie, int nServerId)
			{
				Vector3 v3AIPos = pZombie->GetPosition();
				ImGui::Text("[id=%d] AI Agent Pos  : (%.1f, %.1f, %.1f)", nServerId, v3AIPos.x, v3AIPos.y, v3AIPos.z);
				Vector3 v3ZombiePos = pZombie->GetTransform()->GetPosition();
				ImGui::Text("[id=%d] Transform Pos : (%.1f, %.1f, %.1f)", nServerId, v3ZombiePos.x, v3ZombiePos.y, v3ZombiePos.z);
				auto pCapsuleCollider = pZombie->GetComponent<PlayerCollider>();
				if (pCapsuleCollider)
					ImGui::Text("[id=%d] Capsule Radius : %.1f cm", nServerId, pCapsuleCollider->GetCapsuleWorld().fRadius);
				ImGui::Text("[id=%d] AI State : %d", nServerId, (int)pZombie->GetBehaviorState());
				ImGui::Text("[id=%d] HP : %.0f", nServerId, pZombie->GetHP());
				ImGui::ProgressBar(pZombie->GetHP() / 100.f, ImVec2(-1.f, 0.f));
				if (pZombie->IsDead()) ImGui::TextColored(ImVec4(1, 0, 0, 1), "[id=%d] DEAD", nServerId);

				if (zombieIdx == 0 && m_pNavMeshDebugRenderer)
					m_pNavMeshDebugRenderer->UpdatePathNodes(pZombie->GetPathDebugInfo().Waypoints);

				const auto& debugInfo = pZombie->GetPathDebugInfo();
				ImGui::Separator();
				ImGui::Text("[id=%d] Path Debug Info:", nServerId);
				ImGui::Text("  Start: (%.1f, %.1f, %.1f)", debugInfo.v3StartPos.x, debugInfo.v3StartPos.y, debugInfo.v3StartPos.z);
				ImGui::Text("  End  : (%.1f, %.1f, %.1f)", debugInfo.v3EndPos.x, debugInfo.v3EndPos.y, debugInfo.v3EndPos.z);

				if (!debugInfo.Waypoints.empty()) {
					ImGui::Text("  Waypoints (%d):", (int)debugInfo.Waypoints.size());
					for (int i = 0; i < (int)debugInfo.Waypoints.size() && i < 5; ++i) {
						const auto& wp = debugInfo.Waypoints[i];
						ImGui::Text("    [%d] (%.1f, %.1f, %.1f)", i, wp.x, wp.y, wp.z);
					}
					if (debugInfo.Waypoints.size() > 5)
						ImGui::Text("    ... and %d more", (int)debugInfo.Waypoints.size() - 5);
				}
				++zombieIdx;
			};

			if (bOnlineMode)
			{
				for (auto& [nId, pZombie] : m_ServerZombies)
				{
					if (!pZombie || !pZombie->IsPoolActive()) continue;
					ShowZombieDebug(pZombie, nId);
				}
				if (m_ServerZombies.empty()) ImGui::Text("No Server Zombies");
			}
			else
			{
				for (const auto& pZombie : m_World.GetObjects<Zombie>())
					ShowZombieDebug(pZombie, pZombie->GetServerId());
				if (zombieIdx == 0) ImGui::Text("No Zombies");
			}

			ImGui::Text("====== Collision Result ======");
			for (const auto& pair : m_pCollisionPairs) {
				ImGui::Text("Collision {%s : %s}", pair.pSelf->GetName().c_str(), pair.pOther->GetName().c_str());
			}

			// NavMesh 디버그 렌더링 토글
			ImGui::NewLine();
			ImGui::Text("====== NavMesh Debug ======");
			if (m_pNavMeshDebugRenderer) {
				bool bEnabled = m_pNavMeshDebugRenderer->IsEnabled();
				if (ImGui::Checkbox("Show NavMesh", &bEnabled)) {
					m_pNavMeshDebugRenderer->SetEnabled(bEnabled);
				}
			}
		}
		else {
			ImGui::Text("No Animation");
		}
	}
	ImGui::End();

	//ProcessPlayerShoot();
	ProcessNetworkZombies();
	RemoveDeadZombies();
}

void TestScene::ProcessPlayerShoot()
{
	auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer);
	if (!pPlayer || !pPlayer->ConsumeFire())
		return;

	auto pCamera = std::static_pointer_cast<ThirdPersonCamera>(pPlayer->GetCamera());
	Vector3 v3RayOrigin = pCamera->GetPosition();
	Vector3 v3RayDir = pCamera->GetLook();

	float fMinDist = std::numeric_limits<float>::max();
	std::shared_ptr<Zombie> pHitZombie;

	for (const auto& pZombie : m_World.GetObjects<Zombie>()) {
		auto pCollider = pZombie->GetComponent<PlayerCollider>();
		if (!pCollider) continue;

		BoundingBox aabb;
		pCollider->GetCapsuleWorld().CreateAABBFromCapsule(aabb);

		float fDist = 0.f;
		if (aabb.Intersects(XMLoadFloat3(&v3RayOrigin), XMLoadFloat3(&v3RayDir), fDist)) {
			if (fDist < fMinDist) {
				fMinDist = fDist;
				pHitZombie = pZombie;
			}
		}
	}

	if (pHitZombie)
		pHitZombie->TakeDamage(25.f);
}

void TestScene::RemoveDeadZombies()
{
	m_World.RemoveIfAlive<Zombie>([&](const std::shared_ptr<IGameObject>& obj) {
		auto pZombie = std::dynamic_pointer_cast<Zombie>(obj);
		if (!pZombie || !pZombie->IsReadyToRemove()) {
			return false;
		}

		RemoveCollisionPairsOf(pZombie.get());
		return true;
	});

	// 죽은 좀비를 풀로 반환 (swap-with-last O(1), 소멸자 호출 없음)
	m_ZombiePool.Collect();
}

void TestScene::SpawnZombie()
{
	auto pZombie = m_ZombiePool.Acquire();
	AddObject(pZombie);
	if (!pZombie) return; // 풀 고갈

	pZombie->SetPosition(AI->GetNavMesh()->GetRandomPoint());
	pZombie->SetTarget(m_pPlayer);
}

void TestScene::ProcessNetworkZombies()
{
	// 오프라인이거나 미연결 시 서버 이벤트 처리 생략
	if (!NETWORK->IsConnected() || NETWORK->IsOffline()) return;

	// ── 플레이어 위치 전송 (20Hz 스로틀링은 내부 처리) ─────────────────────
	{
		const Vector3& v3Pos = m_pPlayer->GetTransform()->GetPosition();
		// yaw: Transform에서 Y 회전을 추출 (플레이어가 SetRotation(0,yaw,0) 사용)
		Matrix mtxWorld = m_pPlayer->GetWorldMatrix();
		float fYaw = std::atan2f(mtxWorld._31, mtxWorld._33);
		NETWORK->SetLocalPlayerInfo(v3Pos, fYaw);
		NETWORK->TrySendPlayerPosition();
	}

	// ── 스폰 이벤트 처리 (Task 6) ─────────────────────────────────────────
	for (auto& ev : NETWORK->ConsumeSpawnEvents())
	{
		// 이미 해당 ID로 스폰된 좀비가 있으면 무시
		if (m_ServerZombies.count(ev.zombieId)) continue;

		auto pZombie = m_ZombiePool.Acquire();
		if (!pZombie) break; // 풀 고갈

		pZombie->SetServerId(ev.zombieId);
		pZombie->SetPosition(ev.pos);
		pZombie->SetTarget(m_pPlayer); // 오프라인 AI fallback 대비
		m_ServerZombies[ev.zombieId] = pZombie;
	}

	// ── 디스폰 이벤트 처리 (Task 6) ──────────────────────────────────────
	for (int nId : NETWORK->ConsumeDespawnEvents())
	{
		auto it = m_ServerZombies.find(nId);
		if (it == m_ServerZombies.end()) continue;

		m_ZombiePool.MarkForRelease(it->second);
		m_ServerZombies.erase(it);
	}

	// ── 좀비 상태 적용 (Task 7) ───────────────────────────────────────────
	for (auto& [nId, pZombie] : m_ServerZombies)
	{
		if (!pZombie || !pZombie->IsPoolActive()) continue;

		ZombieServerState state;
		if (NETWORK->GetLatestZombieState(nId, state))
			pZombie->ApplyServerState(state.x, state.z, state.yaw,
			                          state.behaviorState, state.receivedTime);
	}

	// ── 공격 이벤트 처리 (Task 9) ─────────────────────────────────────────
	for (auto& ev : NETWORK->ConsumeAttackEvents())
	{
		// 나 자신이 타겟인 경우에만 데미지 적용
		if (ev.targetPlayerId != NETWORK->GetPlayerID()) continue;

		m_pPlayer->TakeDamage(ev.damage);

		// 공격한 좀비의 애니메이션 재생
		auto it = m_ServerZombies.find(ev.zombieId);
		if (it != m_ServerZombies.end() && it->second)
		{
			auto pAC = it->second->GetComponent<ZombieAnimationController>();
			if (pAC && pAC->GetMontage())
				pAC->GetMontage()->PlayMontage("Zombie Attack");
		}
	}
}
