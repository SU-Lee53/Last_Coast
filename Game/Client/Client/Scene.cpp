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

void Scene::OnPreProcessInput()
{
	// TODO : Cache last frame world transform
	// Reason : to generate motion vector
}

void Scene::OnPostProcessInput()
{
	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
	}

	for (auto& obj : m_pGameObjects) {
		obj->ProcessInput();
	}
}

void Scene::OnPreUpdate()
{
}

void Scene::OnPostUpdate()
{
	if (m_pPlayer) {
		m_pPlayer->Update();
	}

	for (auto& obj : m_pGameObjects) {
		obj->Update();
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

//HRESULT Scene::LoadFromFiles(const std::string& strFileName)
//{
//	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);
//
//	std::ifstream inFile{ strFilePath, std::ios::binary };
//	if (!inFile) {
//		__debugbreak();
//		return E_INVALIDARG;
//	}
//
//	//std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
//	//nlohmann::json j = nlohmann::json::from_bson(bson);;
//
//	nlohmann::json jScene = nlohmann::json::parse(inFile);
//
//	for (const auto& jObject : jScene) {
//		std::shared_ptr<GameObject> pObj = std::make_shared<GameObject>();
//		pObj->SetFrameName(jObject["ActorName"].get<std::string>());
//
//
//		auto locArray = jObject["Transform"]["Location"].get<std::vector<float>>();
//		Vector3 v3Position(locArray.data());
//
//		auto rotArray = jObject["Transform"]["Rotation"].get<std::vector<float>>();
//		Quaternion v3Rotation(rotArray.data());
//
//
//		//Vector3 v3Scale;
//		//v3Scale.x = jObject["Transform"]["Scale"]["X"].get<float>();
//		//v3Scale.y = jObject["Transform"]["Scale"]["Y"].get<float>();
//		//v3Scale.z = jObject["Transform"]["Scale"]["Z"].get<float>();
//
//		pObj->GetTransform().SetPosition(v3Position);
//		pObj->GetTransform().SetRotation(v3Rotation);
//		//pObj->GetTransform().Scale(v3Rotation);
//
//		std::string strMeshName = jObject["MeshName"].get<std::string>();
//		auto pMeshObject = MODEL->LoadOrGet(strMeshName)->CopyObject<GameObject>();
//		pObj->SetChild(pMeshObject);
//
//		m_pGameObjects.push_back(pObj);
//	}
//
//}



HRESULT Scene::LoadFromFiles(const std::string& strFileName)
{
	std::string strFilePath = std::format("{}/{}.json", g_strSceneBasePath, strFileName);

	std::ifstream inFile{ strFilePath, std::ios::binary };
	if (!inFile) {
		__debugbreak();
		return E_INVALIDARG;
	}

	//std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {});
	//nlohmann::json j = nlohmann::json::from_bson(bson);;

	nlohmann::json jScene = nlohmann::json::parse(inFile);

	for (const auto& jObject : jScene) {
		std::shared_ptr<GameObject> pObj = std::make_shared<GameObject>();
		pObj->SetFrameName(jObject["ActorName"].get<std::string>());


		auto locArray = jObject["Transform"]["WorldMatrix"].get<std::vector<float>>();
		Matrix v3Position(locArray.data());




		//Vector3 v3Scale;
		//v3Scale.x = jObject["Transform"]["Scale"]["X"].get<float>();
		//v3Scale.y = jObject["Transform"]["Scale"]["Y"].get<float>();
		//v3Scale.z = jObject["Transform"]["Scale"]["Z"].get<float>();

		pObj->GetTransform().SetWorldMatrix(v3Position);

		std::wstringstream ss;
		ss << L"=== DirectX Loaded Matrix ===" << std::endl;

		Vector3 right = v3Position.Right();
		ss << L"Right(X):   X=" << right.x << L" Y=" << right.y << L" Z=" << right.z << std::endl;

		Vector3 up = v3Position.Up();
		ss << L"Up(Y):      X=" << up.x << L" Y=" << up.y << L" Z=" << up.z << std::endl;

		Vector3 forward = v3Position.Backward();
		ss << L"Forward(Z): X=" << forward.x << L" Y=" << forward.y << L" Z=" << forward.z << std::endl;

		Vector3 position = v3Position.Translation();
		ss << L"Position:   X=" << position.x << L" Y=" << position.y << L" Z=" << position.z << std::endl;

		OutputDebugStringW(ss.str().c_str());


		std::string strMeshName = jObject["MeshName"].get<std::string>();
		auto pMeshObject = MODEL->LoadOrGet(strMeshName)->CopyObject<GameObject>();
		pObj->SetChild(pMeshObject);

		m_pGameObjects.push_back(pObj);
	}

}
