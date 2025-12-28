#include "pch.h"
#include "Scene.h"

void Scene::InitializeObjects()
{
	if (m_pPlayer)
		m_pPlayer->Initialize();

	for (auto& obj : m_pGameObjects) {
		obj->Initialize();
	}
}

void Scene::UpdateObjects()
{
	ProcessInput();

	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
		m_pPlayer->Update();
	}

	for (auto& obj : m_pGameObjects) {
		obj->ProcessInput();
		obj->Update();
	}

}

void Scene::RenderObjects(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_pPlayer)
		m_pPlayer->Render(pd3dCommandList);

	for (auto& pObj : m_pGameObjects) {
		pObj->Render(pd3dCommandList);
	}

	for (auto& pSprite : m_pSprites) {
		pSprite->AddToUI(pSprite->GetLayerIndex());
	}
}

CB_LIGHT_DATA Scene::MakeLightData()
{
	CB_LIGHT_DATA lightData;

	for (int i = 0; i < m_pLights.size(); ++i) {
		lightData.gLights[i] = m_pLights[i]->MakeLightData();
	}

	lightData.gcGlobalAmbientLight = Vector4(1.f, 1.f, 1.f, 1.f);
	lightData.gnLights = m_pLights.size();

	return lightData;
}
