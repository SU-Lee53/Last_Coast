#include "pch.h"
#include "TerrainTestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "TerrainObject.h"
#include "MapTestScene.h"
#include "Skybox.h"

void TerrainTestScene::BuildObjects()
{
	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	m_pPlayer = std::make_shared<ThirdPersonPlayer>();

	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("TEST");

	//m_pPlayer = std::make_shared<DebugPlayer>();
}

void TerrainTestScene::OnEnterScene()
{
}

void TerrainTestScene::OnLeaveScene()
{
}

void TerrainTestScene::ProcessInput()
{
}

void TerrainTestScene::Update()
{
	ImGui::Begin("Test");
	{
		if (ImGui::Button("Change Scene")) {
			SCENE->ChangeScene<MapTestScene>();
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("Debug")) {
			if (ImGui::BeginTabItem("Player")) {
				if (auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(m_pPlayer)) {
					ImGui::Text("Press `(~) to use mouse control");
					ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");

					ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());

					const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
					ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);

					const auto& transform = pPlayer->GetTransform();
					Vector3 v3PlayerPos = transform->GetPosition();
					ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

					const auto& spaceDesc = GetSpacePartition();
					ScenePartition::CellCoord cdPlayer = spaceDesc.WorldToCellXZ(v3PlayerPos);
					int32 cellIndex = spaceDesc.CellToIndex(cdPlayer.x, cdPlayer.y);
					ImGui::NewLine();
					ImGui::Text("====== Space Partition ======");
					ImGui::Text("Player is in (%d, %d) - # %d", cdPlayer.x, cdPlayer.y, cellIndex);

					ImGui::Text("====== Collision Result ======");
					for (const auto& pair : m_pCollisionPairs) {
						ImGui::Text("Collision {%s : %s}", pair.pSelf->GetName().c_str(), pair.pOther->GetName().c_str());
					}

				}
				else {
					ImGui::Text("No Animation");
				}
				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Terrain")) {
				ImGui::DragFloat3("Terrain Position", (float*)&v3TerrainPos);
				ImGui::DragFloat3("Terrain Rotation", (float*)&v3TerrainRotation);

				m_pTerrain->GetTransform()->SetPosition(v3TerrainPos);
				m_pTerrain->GetTransform()->SetRotation(v3TerrainRotation);

				ImGui::EndTabItem();
			}
			
			if (ImGui::BeginTabItem("Skybox")) {
				if (m_pSkybox) {
					m_pSkybox->ShowControllImGui();
				}

				ImGui::EndTabItem();
			}



			ImGui::EndTabBar();
		}

	}
	ImGui::End();

}
