#include "pch.h"
#include "AnimationTestScene.h"
#include "DebugPlayer.h"
#include "ThirdPersonPlayer.h"
#include "TerrainObject.h"
#include "Skybox.h"

void AnimationTestScene::BuildObjects()
{
	m_pSkybox = std::make_shared<Skybox>();
	m_pSkybox->Initialize();

	auto pLight = std::make_shared<DirectionalLight>();
	{
		pLight->m_v3Color = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Direction = Vector3{ 1.f, 1.f, 1.f };
		pLight->m_v3Position = Vector3{ 100.f, 10000.f, 100.f };
		pLight->m_fIntensity = 0.2;

		pLight->m_v3Direction.Normalize();
	}

	m_pLights.reserve(1);
	m_pLights.push_back(pLight);

	m_pPlayer = std::make_shared<LocalThirdPersonPlayer>();
	//m_pPlayer = std::make_shared<DebugPlayer>();
	//LoadFromFiles("LightTest");

	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("TEST");

	//m_pPlayer = std::make_shared<DebugPlayer>();

	/*
	m_pPlayer = std::make_shared<DebugPlayer>();
	
	m_pTerrain = std::make_shared<TerrainObject>();
	m_pTerrain->LoadFromFiles("TEST");
	
	v3TerrainPos = m_pTerrain->GetTransform()->GetPosition();
	*/
}

void AnimationTestScene::OnEnterScene()
{
}

void AnimationTestScene::OnLeaveScene()
{
}

void AnimationTestScene::ProcessInput()
{
}

void AnimationTestScene::Update()
{
	//ImGui::Begin("Test");
	//{
	//	if (m_pGameObjects[0]->GetAnimationController()) {
	//		const auto& SRTs = m_pGameObjects[0]->GetAnimationController()->GetFinalOutput();
	//		double fTime = m_pGameObjects[0]->GetAnimationController()->GetElapsedTime();
	//		double dDuration = m_pGameObjects[0]->GetAnimationController()->GetCurrentAnimationDuration();
	//		ImGui::Text("ElapsedTime : %f", fTime);0530
	//		ImGui::Text("Duration : %f", dDuration);
	//		for (int i = 0; i < SRTs.size(); ++i) {
	//			if (ImGui::TreeNode(std::format("Animation SRT#{}", i).c_str())) {
	//				ImGui::Text("R1 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._11, SRTs[i]._12, SRTs[i]._13, SRTs[i]._14);
	//				ImGui::Text("R2 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._21, SRTs[i]._22, SRTs[i]._23, SRTs[i]._24);
	//				ImGui::Text("R3 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._31, SRTs[i]._32, SRTs[i]._33, SRTs[i]._34);
	//				ImGui::Text("R4 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._41, SRTs[i]._42, SRTs[i]._43, SRTs[i]._44);
	//
	//				ImGui::TreePop();
	//			}
	//		}
	//	}
	//	else {
	//		ImGui::Text("No Animation");
	//	}
	//}
	//ImGui::End();
	
	ImGui::Begin("Test");
	{
		if (auto pPlayer = std::static_pointer_cast<IThirdPersonPlayer>(m_pPlayer)) {
			ImGui::Text("Press `(~) to use mouse control");
			ImGui::Text("Mouse : %s", pPlayer->IsMouseOn() ? "ON" : "OFF");
	
			ImGui::Text("Move Speed : %f\n", pPlayer->GetMoveSpeed());
	
			const Vector3& v3PlayerMoveDirection = pPlayer->GetMoveDirection();
			ImGui::Text("Move Direction : (%f, %f, %f)", v3PlayerMoveDirection.x, v3PlayerMoveDirection.y, v3PlayerMoveDirection.z);
	
			const auto& transform = pPlayer->GetTransform();
			Vector3 v3PlayerPos = transform->GetPosition();
			ImGui::Text("Player Position : (%f, %f, %f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);
	
			/*const auto& cellData = spaceDesc.GetCellData(cdPlayer);
			if (cellData) {
				for (uint32 i = 0; i < cellData->pObjectsInCell.size(); ++i) {
					ImGui::Text("# %d - Name : %s", i, cellData->pObjectsInCell[i]->GetName().c_str());
				}
			}
			else {
				ImGui::Text("Out of Range");
			}*/
		}
		else {
			ImGui::Text("No Animation");
		}
	}
	ImGui::End();

	ImGui::Begin("Lights Controller");
	{
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
	}
	ImGui::End();

	ImGui::Begin("Skybox Controller");
	{
		if (m_pSkybox) {
			m_pSkybox->ShowControllImGui();
		}
	}
	ImGui::End();


	//ImGui::Begin("Test");
	//{
	//	ImGui::DragFloat3("Terrain Position", (float*)&v3TerrainPos);
	//	ImGui::DragFloat3("Terrain Rotation", (float*)&v3TerrainRotation);
	//
	//	m_pTerrain->GetTransform()->SetPosition(v3TerrainPos);
	//	m_pTerrain->GetTransform()->SetRotation(v3TerrainRotation);
	//}
	//ImGui::End();

}
