#include "pch.h"
#include "NetworkGameTestScene.h"
#include "Skybox.h"
#include "TerrainObject.h"
#include "ThirdPersonPlayer.h"

void NetworkGameTestScene::BuildObjects()
{
	m_pUIBoard = std::make_unique<UIBoard>();

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	m_pPlayer = std::make_shared<NetworkOwnerThirdPersonPlayer>();
	m_pPlayer->Initialize();

	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("Game");

	if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		pThirdPerson->GiveWeapon(WEAPON_TYPE::AK);
	}

	//LoadFromFiles("Game");
}

void NetworkGameTestScene::OnEnterScene()
{

}

void NetworkGameTestScene::OnLeaveScene()
{

}

void NetworkGameTestScene::ProcessInput()
{
}

void NetworkGameTestScene::Update()
{
	ImGui::Begin("Test");
	{
		if (ImGui::Button("Move To PlayerStart")) {
			m_pPlayer->GetTransform()->SetPosition(10281.199179, -3536.692724, 18949.001705);
		}
		if (ImGui::BeginTabBar("Debug")) {
			if (ImGui::BeginTabItem("Player")) {
				if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
					m_pPlayer->ShowControlImGui();

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

					//m_pGun->ShowControlImGui();
				}
				else {
					ImGui::Text("No Animation");
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lights")) {
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
