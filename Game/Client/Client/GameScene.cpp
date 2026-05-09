#include "pch.h"
#include "GameScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "TerrainObject.h"
#include "TerrainTestScene.h"
#include "Skybox.h"
#include "MapTestScene.h"
#include "TextBox.h"

void GameScene::BuildObjects()
{
	using namespace std::chrono;

	m_pUIBoard = std::make_unique<UIBoard>();
	m_pPlayer = std::make_shared<LocalThirdPersonPlayer>();
	m_pPlayer->Initialize();
	if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		pThirdPerson->GiveWeapon(WEAPON_TYPE::AK);
	}

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	//m_pPlayer = std::make_shared<DebugPlayer>();
	//m_pPlayer->Initialize();

	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("Game");

	auto begin = high_resolution_clock::now();
	LoadFromFiles("Game");
	auto end = high_resolution_clock::now();
	long long llLoadTime = duration_cast<milliseconds>(end - begin).count();

	std::shared_ptr<TextBox> pText = std::make_shared<TextBox>(L"Malgun Gothic");
	pText->SetText(std::format(L"로딩 시간 : {}ms", llLoadTime));
	pText->SetLayer(0);
	pText->SetAnchor(Vector2{ 0,0 });
	pText->SetPivot(Vector2{ 0,0 });
	pText->SetPosition(Vector2{ 10,150 });
	pText->SetSize(Vector2{ 250,50 });
	m_pUIBoard->InsertUI(pText);

}

void GameScene::OnEnterScene()
{
}

void GameScene::OnLeaveScene()
{
}

void GameScene::ProcessInput()
{
}

void GameScene::Update()
{
	ImGui::Begin("Test");
	{
		if (ImGui::Button("Change Scene")) {
			SCENE->ChangeScene<MapTestScene>();
			ImGui::End();
			return;
		}

		ImGui::InputFloat3("Set Pos", reinterpret_cast<float*>(&v3PlayerPos));
		if (ImGui::Button("Move To Pos")) {
			m_pPlayer->GetTransform()->SetPosition(v3PlayerPos);
		}
	
		if (ImGui::Button("Move To PlayerStart")) {
			m_pPlayer->GetTransform()->SetPosition(10281.199179, -3536.692724, 18949.001705);
		}
	
		if (ImGui::BeginTabBar("Debug")) {
			if (ImGui::BeginTabItem("Player")) {
				if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
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
	
			if (ImGui::BeginTabItem("Objects")) {
				for (const auto& pObj : m_World.GetObjects<StaticObject>()) {
					if (ImGui::TreeNode(pObj->GetName().c_str())) {
						auto pTransform = pObj->GetTransform();
						const Vector3 v3Position = pTransform->GetPosition();
						ImGui::Text("Position : (%f, %f, %f)", v3Position.x, v3Position.y, v3Position.z);

						pObj->ShowControlImGui();

						ImGui::TreePop();
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Terrain")) {
				ImGui::DragFloat3("Terrain Position", (float*)&v3TerrainPos, 0.1f);
				ImGui::DragFloat3("Terrain Rotation", (float*)&v3TerrainRotation, 0.1f);

				m_pTerrain->GetTransform()->SetPosition(v3TerrainPos);
				m_pTerrain->GetTransform()->SetRotation(v3TerrainRotation);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	
	}
	ImGui::End();

}
