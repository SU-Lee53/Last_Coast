#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "Zombie.h"
#include "Skybox.h"

void TestScene::BuildObjects()
{
	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize("Day_HDRI.dds", "Night_HDRI.dds");

	m_pPlayer = std::make_shared<ThirdPersonPlayer>();

	for (int i = 0; i < 10; ++i) {
		std::shared_ptr<IGameObject> pObj = std::make_shared<Zombie>();
		m_pGameObjects.push_back(pObj);
	}

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
		if (auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(m_pPlayer)) {
			ImGui::Text("Press `(~) to use mouse control");
			ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");
			ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());
			const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
			ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);
			const auto& transform = pPlayer->GetTransform();
			Vector3 v3PlayerPos = transform->GetPosition();
			ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

			// ★ 좀비 위치 출력
			ImGui::NewLine();
			ImGui::Text("====== Zombie ======");
			int zombieIdx = 0;
			for (const auto& obj : m_pGameObjects) {
				if (auto pZombie = std::dynamic_pointer_cast<Zombie>(obj)) {
					Vector3 v3AIPos = pZombie->GetPosition(); // AIAgent 기준 위치
					ImGui::Text("[%d] AI Agent Pos  : (%.1f, %.1f, %.1f)", zombieIdx, v3AIPos.x, v3AIPos.y, v3AIPos.z);
					Vector3 v3ZombiePos = pZombie->GetTransform()->GetPosition();
					ImGui::Text("[%d] Transform Pos : (%.1f, %.1f, %.1f)", zombieIdx, v3ZombiePos.x, v3ZombiePos.y, v3ZombiePos.z);
					ImGui::Text("[%d] AI State : %d", zombieIdx,
						(int)pZombie->GetBehaviorState()); // Idle=0, PathRequested=1, Moving=2

					// 경로 디버그 정보
					// A* 경로 노드를 디버그 렌더러에 반영 (첫 번째 좀비 기준)
				if (zombieIdx == 0 && m_pNavMeshDebugRenderer) {
					m_pNavMeshDebugRenderer->UpdatePathNodes(pZombie->GetPathDebugInfo().Waypoints);
				}

				const auto& debugInfo = pZombie->GetPathDebugInfo();
					ImGui::Separator();
					ImGui::Text("[%d] Path Debug Info:", zombieIdx);
					ImGui::Text("  Start: (%.1f, %.1f, %.1f)", debugInfo.v3StartPos.x, debugInfo.v3StartPos.y, debugInfo.v3StartPos.z);
					ImGui::Text("  End  : (%.1f, %.1f, %.1f)", debugInfo.v3EndPos.x, debugInfo.v3EndPos.y, debugInfo.v3EndPos.z);

					if (!debugInfo.Waypoints.empty()) {
						ImGui::Text("  Waypoints (%d):", (int)debugInfo.Waypoints.size());
						for (int i = 0; i < (int)debugInfo.Waypoints.size() && i < 10; ++i) {
							const auto& wp = debugInfo.Waypoints[i];
							ImGui::Text("    [%d] (%.1f, %.1f, %.1f)", i, wp.x, wp.y, wp.z);
						}
						if (debugInfo.Waypoints.size() > 10)
							ImGui::Text("    ... and %d more", (int)debugInfo.Waypoints.size() - 10);
					}

					if (!debugInfo.Portals.empty()) {
						ImGui::Text("  Portals (%d):", (int)debugInfo.Portals.size());
						for (int i = 0; i < (int)debugInfo.Portals.size() && i < 10; ++i) {
							const auto& p = debugInfo.Portals[i];
							ImGui::Text("    [%d] Poly %d->%d", i, p.nPolyA, p.nPolyB);
							ImGui::Text("        L(%.1f, %.1f)  R(%.1f, %.1f)",
								p.v3Left.x, p.v3Left.z,
								p.v3Right.x, p.v3Right.z);
						}
						if (debugInfo.Portals.size() > 10)
							ImGui::Text("    ... and %d more", (int)debugInfo.Portals.size() - 10);
					}
					if (!debugInfo.PolyNodeCenters.empty()) {
						ImGui::Text("  PolyNodeCenters (%d):", (int)debugInfo.PolyNodeCenters.size());
						for (int i = 0; i < (int)debugInfo.PolyNodeCenters.size() && i < 10; ++i) {
							const auto& c = debugInfo.PolyNodeCenters[i];
							ImGui::Text("    [%d] (%.1f, %.1f, %.1f)", i, c.x, c.y, c.z);
						}
						if (debugInfo.PolyNodeCenters.size() > 10)
							ImGui::Text("    ... and %d more", (int)debugInfo.PolyNodeCenters.size() - 10);
					}
					++zombieIdx;
				}
			}
			if (zombieIdx == 0) ImGui::Text("No Zombies");

			const auto& spaceDesc = GetSpacePartitionDesc();
			SpacePartitionDesc::CellCoord cdPlayer = spaceDesc.WorldToCellXZ(v3PlayerPos);
			int32 cellIndex = spaceDesc.CellToIndex(cdPlayer.x, cdPlayer.y);
			ImGui::NewLine();
			ImGui::Text("====== Space Partition ======");
			ImGui::Text("Player is in (%d, %d) - # %d", cdPlayer.x, cdPlayer.y, cellIndex);
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

}
