#include "pch.h"
#include "MapTestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "Skybox.h"
#include "GameScene.h"
#include "TextBox.h"
#include "WeaponObject.h"

void MapTestScene::BuildObjects()
{
	m_pUIBoard = std::make_unique<UIBoard>();

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	m_pPlayer = std::make_shared<ThirdPersonPlayer>();
	m_pPlayer->Initialize();

	//m_pPlayer = std::make_shared<DebugPlayer>();
	//m_pPlayer->Initialize();

	m_pFPSText = std::make_shared<TextBox>(L"Malgun Gothic");
	m_pFPSText->SetText(L"Test");
	m_pFPSText->SetLayer(0);
	m_pFPSText->SetAnchor(Vector2{ 0,0 });
	m_pFPSText->SetPivot(Vector2{ 0,0 });
	m_pFPSText->SetPosition(Vector2{ 0,50 });
	m_pFPSText->SetSize(Vector2{ 100,50 });

	m_pTimeText = std::make_shared<TextBox>(L"Malgun Gothic");
	m_pTimeText->SetText(L"Test");
	m_pTimeText->SetLayer(0);
	m_pTimeText->SetAnchor(Vector2{ 0,0 });
	m_pTimeText->SetPivot(Vector2{ 0,0 });
	m_pTimeText->SetPosition(Vector2{ 200,50 });
	m_pTimeText->SetSize(Vector2{ 100,50 });

	m_pKoreanText = std::make_shared<TextButton>(L"Malgun Gothic");
	m_pKoreanText->SetText(L"클릭하면 글자색이 바뀜");
	m_pKoreanText->SetLayer(0);
	m_pKoreanText->SetAnchor(Vector2{ 0,0 });
	m_pKoreanText->SetPivot(Vector2{ 0,0 });
	m_pKoreanText->SetPosition(Vector2{ 350,50 });
	m_pKoreanText->SetSize(Vector2{ 250,50 });
	
	m_pKoreanText->SetBeginHoverCallback(
		[](IUIComponent* pComp) {
			auto pText = static_cast<TextBox*>(pComp);
			pText->SetColor(Vector3(1,0,0));
		}
	);

	m_pKoreanText->SetEndHoverCallback(
		[](IUIComponent* pComp) {
			auto pText = static_cast<TextBox*>(pComp);
			pText->SetColor(Vector3(1,1,1));
		}
	);

	m_pKoreanText->SetButtonCallback(
		[](IUIComponent* pComp) {
			auto pText = static_cast<TextBox*>(pComp);
			pText->SetColor(RandomGenerator::GenerateRandomColor3());
		}
	);

	m_pUIBoard->InsertUI(m_pFPSText);
	m_pUIBoard->InsertUI(m_pTimeText);
	m_pUIBoard->InsertUI(m_pKoreanText);

	if (auto pThirdPerson = std::dynamic_pointer_cast<ThirdPersonPlayer>(m_pPlayer)) {
		pThirdPerson->GiveWeapon(static_cast<WEAPON_TYPE>(m_nWeaponSelected));
	}

	LoadFromFiles("LightTest");
}

void MapTestScene::OnEnterScene()
{
}

void MapTestScene::OnLeaveScene()
{
}

void MapTestScene::ProcessInput()
{
}

void MapTestScene::Update()
{

	// Update text test
	std::wstring wstrFrameRate;
	wstrFrameRate = std::format(L"FPS : {}", TIME->GetCurrentFrameRate());
	m_pFPSText->SetText(wstrFrameRate);

	std::wstring wstrTime;
	wstrTime = std::format(L"TIME : {:.3f}", TIME->GetTotalTime());
	m_pTimeText->SetText(wstrTime);


	ImGui::Begin("Test");
	{
		if (ImGui::Button("Change Scene")) {
			SCENE->ChangeScene<GameScene>();
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("Debug")) {
			if (ImGui::BeginTabItem("Player")) {
				if (auto pPlayer = std::static_pointer_cast<ThirdPersonPlayer>(m_pPlayer)) {
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

					ImGui::Text("====== Weapon Test ======");
					const auto& strWeaponNames = GameContext::g_strWeaponName;
					if(ImGui::BeginCombo("Weapons", strWeaponNames[m_nWeaponSelected].c_str())) {
						for (int i = 0; i < strWeaponNames.size(); ++i) {
							bool bSelected = (m_nWeaponSelected == i);
							if (ImGui::Selectable(strWeaponNames[i].c_str(), bSelected)) {
								m_nWeaponSelected = i;
								if (auto pThirdPerson = std::dynamic_pointer_cast<ThirdPersonPlayer>(m_pPlayer)) {
									pThirdPerson->GiveWeapon(static_cast<WEAPON_TYPE>(m_nWeaponSelected));
								}
							}

							if (bSelected) {
								ImGui::SetItemDefaultFocus();
							}

						}



						ImGui::EndCombo();
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
