#include "pch.h"
#include "AnimationTestScene.h"
#include "DebugPlayer.h"

void AnimationTestScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	std::shared_ptr<GameObject> pGameObject1 = MODEL->Get("Ch33_nonPBR");
	std::shared_ptr<AnimationController> pAnimationCtrl = std::make_shared<PlayerAnimationController>();
	pGameObject1->SetAnimationController(pAnimationCtrl);
	m_pGameObjects.push_back(pGameObject1);

	std::shared_ptr<GameObject> pGameObject2 = MODEL->Get("vintage_wooden_sniper_optimized_for_fpstps");
	m_pGameObjects.push_back(pGameObject2);

	LoadFromFiles("TEST");

	Scene::InitializeObjects();
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
	ImGui::Begin("Test");
	{
		if (m_pGameObjects[0]->GetAnimationController()) {
			const auto& SRTs = m_pGameObjects[0]->GetAnimationController()->GetFinalOutput();
			double fTime = m_pGameObjects[0]->GetAnimationController()->GetElapsedTime();
			double dDuration = m_pGameObjects[0]->GetAnimationController()->GetCurrentAnimationDuration();
			ImGui::Text("ElapsedTime : %f", fTime);
			ImGui::Text("Duration : %f", dDuration);
			for (int i = 0; i < SRTs.size(); ++i) {
				if (ImGui::TreeNode(std::format("Animation SRT#{}", i).c_str())) {
					ImGui::Text("R1 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._11, SRTs[i]._12, SRTs[i]._13, SRTs[i]._14);
					ImGui::Text("R2 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._21, SRTs[i]._22, SRTs[i]._23, SRTs[i]._24);
					ImGui::Text("R3 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._31, SRTs[i]._32, SRTs[i]._33, SRTs[i]._34);
					ImGui::Text("R4 : %.3f, %.3f, %.3f, %.3f", SRTs[i]._41, SRTs[i]._42, SRTs[i]._43, SRTs[i]._44);

					ImGui::TreePop();
				}
			}
		}
		else {
			ImGui::Text("No Animation");
		}
	}
	ImGui::End();


}

void AnimationTestScene::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList)
{
	Scene::RenderObjects(pd3dCommansList);
}
