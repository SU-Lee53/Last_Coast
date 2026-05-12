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
		pThirdPerson->GiveWeapon(WEAPON_TYPE::M4);
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

			if (ImGui::BeginTabItem("Remote Players")) {
				for (auto& [id, player] : m_RemotePlayers) {
					ImGui::Text("Player ID: %d", id);
					ImGui::Text(" - Speed: %.2f", player->GetMoveSpeed());
					ImGui::Text(" - Moved: %s", player->IsMoving() ? "TRUE" : "FALSE");
					ImGui::Separator();
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

	SyncSceneWithServer();
}

void NetworkGameTestScene::SyncSceneWithServer()
{
	for (auto& [id, player] : m_RemotePlayers) {
		if (auto remote = std::dynamic_pointer_cast<NetworkRemoteThirdPersonPlayer>(player)) {
			if (TIME->GetTotalTime() - remote->GetLastPacketTime() > 0.1f) {
				remote->ResetMovementState();
			}
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerJoins()) {

		if (m_RemotePlayers.contains(ev.playerId)) continue;

		auto remotePlayer = std::make_shared<NetworkRemoteThirdPersonPlayer>();
		remotePlayer->Initialize();

		remotePlayer->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.initialTransform.m), ev.bRunning, ev.bAiming, ev.fAimPitch);
		remotePlayer->GiveWeapon(WEAPON_TYPE::AK);
		AddObject(remotePlayer);
		m_RemotePlayers[ev.playerId] = remotePlayer;
	}

	for (auto id : NETWORK->ConsumePlayerLeaves()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			RemoveObject(it->second);
			m_RemotePlayers.erase(it);
		}
	}

	for (auto& ev : NETWORK->ConsumePlayerTransforms()) {
		auto it = m_RemotePlayers.find(ev.playerId);
		if (it != m_RemotePlayers.end()) {
			it->second->UpdateNetworkTransform(reinterpret_cast<Matrix&>(ev.transform.m), ev.bRunning, ev.bAiming, ev.fAimPitch);
		}
	}

	for (auto id : NETWORK->ConsumePlayerReloads()) {
		auto it = m_RemotePlayers.find(id);
		if (it != m_RemotePlayers.end()) {
			std::cout << "[Scene] Playing Reload for Remote Player " << id << std::endl;
			it->second->PlayReloadStartAction();
		}
	}
}
