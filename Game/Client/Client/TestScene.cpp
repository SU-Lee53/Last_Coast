#include "pch.h"
#include "TestScene.h"
#include "DebugPlayer.h"

void TestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	std::shared_ptr<GameObject> pGameObject = MODEL->Get("Ch33_nonPBR");
	std::shared_ptr<AnimationController> pAnimationCtrl = std::make_shared<PlayerAnimationController>();
	pGameObject->SetAnimationController(pAnimationCtrl);
	
	m_pGameObjects.push_back(pGameObject);

	Scene::InitializeObjects();
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
		const auto& SRTs = m_pGameObjects[0]->GetAnimationController()->GetKeyframeSRT();
		double dTime = m_pGameObjects[0]->GetAnimationController()->GetElapsedTime();
		double dDuration = m_pGameObjects[0]->GetAnimationController()->GetCurrentAnimationDuration();
		ImGui::Text("ElapsedTime : %f", dTime);
		ImGui::Text("Duration : %f", dDuration);
		for (int i = 0; i < SRTs.size(); ++i) {
			if(ImGui::TreeNode(std::format("Animation SRT#{}", i).c_str())) {
				ImGui::Text("R1 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._11, SRTs[i]._12, SRTs[i]._13, SRTs[i]._14);
				ImGui::Text("R2 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._21, SRTs[i]._22, SRTs[i]._23, SRTs[i]._24);
				ImGui::Text("R3 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._31, SRTs[i]._32, SRTs[i]._33, SRTs[i]._34);
				ImGui::Text("R4 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._41, SRTs[i]._42, SRTs[i]._43, SRTs[i]._44);

				ImGui::TreePop();
			}
		}

	}
	ImGui::End();


}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	Scene::RenderObjects(pd3dCommansList);
}
