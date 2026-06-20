#include "pch.h"
#include "MapTestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "Skybox.h"
#include "GameScene.h"
#include "TextBox.h"
#include "WeaponObject.h"
#include "EventSequence.h"
#include "ExplosionEffect.h"
#include "FireEffect.h"
#include "SmokeEffect.h"

void MapTestScene::BuildObjects()
{
	m_pUIBoard = std::make_unique<UIBoard>();

	m_v4GlobalAmbient = Vector4(0.2f, 0.2f, 0.2f, 1.f);

	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	m_pPlayer = std::make_shared<LocalThirdPersonPlayer>();
	m_pPlayer->Initialize();

	//m_pPlayer = std::make_shared<DebugPlayer>();
	//m_pPlayer->Initialize();

	m_pFPSText = std::make_shared<TextBox>(L"Malgun Gothic");
	m_pFPSText->SetText(L"Test");
	m_pFPSText->SetLayer(0);
	m_pFPSText->SetAnchor(Vector2{ 0,0 });
	m_pFPSText->SetPivot(Vector2{ 0,0 });
	m_pFPSText->SetPosition(Vector2{ 0,50 });
	m_pFPSText->SetTextHeight(50);

	m_pTimeText = std::make_shared<TextBox>(L"Malgun Gothic");
	m_pTimeText->SetText(L"Test");
	m_pTimeText->SetLayer(0);
	m_pTimeText->SetAnchor(Vector2{ 0,0 });
	m_pTimeText->SetPivot(Vector2{ 0,0 });
	m_pTimeText->SetPosition(Vector2{ 200,50 });
	m_pTimeText->SetTextHeight(50);

	m_pKoreanText = std::make_shared<TextButton>(L"Malgun Gothic");
	m_pKoreanText->SetText(L"카메라 재생");
	m_pKoreanText->SetLayer(0);
	m_pKoreanText->SetAnchor(Vector2{ 0,0 });
	m_pKoreanText->SetPivot(Vector2{ 0,0 });
	m_pKoreanText->SetPosition(Vector2{ 450,50 });
	m_pKoreanText->SetTextHeight(50);

	m_pInputTest = std::make_shared<InputTextBox>(L"Malgun Gothic");
	m_pInputTest->SetLayer(0);
	m_pInputTest->SetAnchor(Vector2{ 0,0 });
	m_pInputTest->SetPivot(Vector2{ 0,0 });
	m_pInputTest->SetPosition(Vector2{ 50,100 });
	m_pInputTest->SetTextHeight(50);

	m_pPWInputTest = std::make_shared<InputTextBox>(L"Malgun Gothic");
	m_pPWInputTest->SetLayer(0);
	m_pPWInputTest->SetAnchor(Vector2{ 0,0 });
	m_pPWInputTest->SetPivot(Vector2{ 0,0 });
	m_pPWInputTest->SetPosition(Vector2{ 50,150 });
	m_pPWInputTest->SetTextHeight(50);
	m_pPWInputTest->SetPasswordMode(true);
	
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

	m_pInputTest->SetEnterCallback(
		[&](IUIComponent* pComp) {
			GetUIBoard()->ClearFocus();
		}
	);

	m_pPWInputTest->SetEnterCallback(
		[&](IUIComponent* pComp) {
			GetUIBoard()->ClearFocus();
		}
	);

	m_pUIBoard->InsertUI(m_pFPSText);
	m_pUIBoard->InsertUI(m_pTimeText);
	m_pUIBoard->InsertUI(m_pKoreanText);
	m_pUIBoard->InsertUI(m_pInputTest);
	m_pUIBoard->InsertUI(m_pPWInputTest);

	if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
		pThirdPerson->GiveWeapon(static_cast<WEAPON_TYPE>(m_nWeaponSelected));
	}

	LoadFromFiles("LightTest");


	m_pEventSequence = std::make_shared<EventSequence>(this);
	m_pEventSequence->AddEvent(std::make_shared<TimeForwardEvent>());
	m_pEventSequence->AddEvent(std::make_shared<BleedEvent>());

	auto pCinematicEvent = std::make_shared<CinematicCameraEvent>();

	m_pEventSequence->AddEvent(std::make_shared<CinematicCameraEvent>());

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
	// model change test
	static std::string pstrModelName[] = {
		"player_m_01",	// not ready
		"player_f_01",	// not ready
		"player_m_02",
		"player_f_02",
	};

	static int32 nModelIndex = 2;
	if (INPUT->GetButtonDown(VK_NEXT)) {
		nModelIndex = ((nModelIndex - 1) % _countof(pstrModelName));
		if (auto p = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			p->SetPlayerModel(pstrModelName[nModelIndex]);
		}
	}
	if (INPUT->GetButtonDown(VK_PRIOR)) {
		nModelIndex = ((nModelIndex + 1) % _countof(pstrModelName));
		if (auto p = static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			p->SetPlayerModel(pstrModelName[nModelIndex]);
		}
	}


	// Update text test
	std::wstring wstrFrameRate;
	wstrFrameRate = std::format(L"FPS : {}", TIME->GetCurrentFrameRate());
	m_pFPSText->SetText(wstrFrameRate);

	std::wstring wstrTime;
	wstrTime = std::format(L"TIME : {:.3f}", TIME->GetTotalTime());
	m_pTimeText->SetText(wstrTime);

	ImGui::Begin("Particle Test");
	{
		if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			const auto& transform = pPlayer->GetTransform();
			Vector3 v3PlayerPos = transform->GetPosition();

			auto MakeParticleSpawnDesc = [&]() {
				ParticleEffectSpawnDesc desc{};
				desc.v3Direction = transform->GetLook();
				desc.v3Direction.Normalize();
				desc.v3Normal = Vector3::Up;
				desc.v3Position = v3PlayerPos + desc.v3Direction * 180.0f;
				desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
				return desc;
				};

			ImGui::Text("Spawn Position : (%.1f, %.1f, %.1f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

			if (ImGui::Button("Spawn Explosion")) {
				PARTICLE->Spawn<ExplosionEffect>(MakeParticleSpawnDesc());
			}
			ImGui::SameLine();
			if (ImGui::Button("Spawn Fire")) {
				ParticleEffectSpawnDesc desc = MakeParticleSpawnDesc();
				desc.v3Direction = Vector3::Up;
				desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
				PARTICLE->Spawn<FireEffect>(desc);
			}
			ImGui::SameLine();
			if (ImGui::Button("Spawn Smoke")) {
				ParticleEffectSpawnDesc desc = MakeParticleSpawnDesc();
				desc.v3Direction = Vector3::Up;
				desc.mtxWorld = Matrix::CreateWorld(desc.v3Position, desc.v3Direction, desc.v3Normal);
				PARTICLE->Spawn<SmokeEffect>(desc);
			}
		}
	}
	ImGui::End();


	//	ImGui::Begin("Test");
	//	{
	//		if (ImGui::Button("Change Scene")) {
	//			SCENE->ChangeScene<GameScene>();
	//			ImGui::End();
	//			return;
	//		}
	//	
	//		if (ImGui::BeginTabBar("Debug")) {
	//			if (ImGui::BeginTabItem("Player")) {
	//				if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
	//					m_pPlayer->ShowControlImGui();
	//	
	//					ImGui::Text("Press `(~) to use mouse control");
	//					ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");
	//	
	//					ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());
	//	
	//					const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
	//					ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);
	//	
	//					const auto& transform = pPlayer->GetTransform();
	//					Vector3 v3PlayerPos = transform->GetPosition();
	//					ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);
	//	
	//					ImGui::Text("====== Collision Result ======");
	//					for (const auto& pair : m_pCollisionPairs) {
	//						ImGui::Text("Collision {%s : %s}", pair.pSelf->GetName().c_str(), pair.pOther->GetName().c_str());
	//					}
	//	
	//					ImGui::Text("====== Weapon Test ======");
	//					const auto& strWeaponNames = GameContext::g_strWeaponNames;
	//					if(ImGui::BeginCombo("Weapons", strWeaponNames[m_nWeaponSelected].c_str())) {
	//						for (int i = 0; i < strWeaponNames.size(); ++i) {
	//							bool bSelected = (m_nWeaponSelected == i);
	//							if (ImGui::Selectable(strWeaponNames[i].c_str(), bSelected)) {
	//								m_nWeaponSelected = i;
	//								if (auto pThirdPerson = std::dynamic_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
	//									pThirdPerson->GiveWeapon(static_cast<WEAPON_TYPE>(m_nWeaponSelected));
	//								}
	//							}
	//	
	//							if (bSelected) {
	//								ImGui::SetItemDefaultFocus();
	//							}
	//	
	//						}
	//	
	//						ImGui::EndCombo();
	//					}
	//	
	//	
	//					//m_pGun->ShowControlImGui();
	//				}
	//				else {
	//					ImGui::Text("No Animation");
	//				}
	//				ImGui::EndTabItem();
	//			}
	//			if (ImGui::BeginTabItem("Lights")) {
	//				ImGui::Text("Elapsed TIme : %f", TIME->GetTimeElapsed());
	//				ImGui::Text("Total TIme : %f", TIME->GetTotalTime());
	//	
	//				float fAmbient = m_v4GlobalAmbient.x;
	//				ImGui::DragFloat("GlobalAmbient", (float*)&fAmbient, 0.001f, 0.f, 1.f);
	//				m_v4GlobalAmbient = XMVectorReplicate(fAmbient);
	//				ImGui::Text("NumLights : %d", m_pLights.size());
	//				for (uint32 i = 0; i < m_pLights.size(); ++i) {
	//					if (ImGui::TreeNode(std::format("Index : {}", i).c_str())) {
	//						m_pLights[i]->ShowControllImGui();
	//						ImGui::TreePop();
	//					}
	//				}
	//				ImGui::EndTabItem();
	//			}
	//			if (ImGui::BeginTabItem("Skybox")) {
	//				if (m_pSkybox) {
	//					m_pSkybox->ShowControllImGui();
	//				}
	//				ImGui::EndTabItem();
	//			}
	//	
	//			ImGui::EndTabBar();
	//		}
	//	
	//	}
	//	ImGui::End();

}
