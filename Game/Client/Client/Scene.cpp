#include "pch.h"
#include "Scene.h"
#include "TerrainObject.h"
#include "StaticObject.h"
#include "NodeObject.h"
#include "Collider.h"

void Scene::InitializeObjects()
{
	if (m_pPlayer) {
		m_pPlayer->Initialize();
	}

	if (m_pTerrain) {
		m_pTerrain->Initialize();
	}

	for (auto& obj : m_pGameObjects) {
		obj->Initialize();
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

void Scene::PostInitialize()
{
	InitializeObjects();
	GenerateSceneBound();
	if (m_pTerrain) {
		Vector3 v3TerrainPos = m_pTerrain->GetComponent<Transform>()->GetPosition();
		uint32 unComponents = std::sqrt(m_pTerrain->GetTerrainComponents().size());
		Vector2 v2CellSize = m_pTerrain->GetTerrainComponents()[0]->GetComponentSize();
		CellPartition(Vector2{v3TerrainPos.x, v3TerrainPos.z}, v2CellSize, unComponents, unComponents);
	}
	else {
		CellPartition(Vector2{ g_fWorldMinX, g_fWorldMinZ }, Vector2{100_m, 100_m}, 10, 10);
	}
}

void Scene::PreProcessInput()
{
	// TODO : Cache last frame world transform
	// Reason : to generate motion vector
}

void Scene::PostProcessInput()
{
	if (m_pPlayer) {
		m_pPlayer->ProcessInput();
	}

	for (auto& obj : m_pGameObjects) {
		obj->ProcessInput();
	}
}

void Scene::PreUpdate()
{
	if (m_pPlayer) {
		m_pPlayer->PreUpdate();
	}

	for (auto& obj : m_pGameObjects) {
		obj->PreUpdate();
	}
}

void Scene::PostUpdate()
{
	// Update
	if (m_pPlayer) {
		m_pPlayer->Update();
	}

	if (m_pTerrain) {
		m_pTerrain->Update();
	}

	for (auto& obj : m_pGameObjects) {
		obj->Update();
	}

	// Post Update
	if (m_pPlayer) {
		m_pPlayer->PostUpdate();
	}

	if (m_pTerrain) {
		m_pTerrain->PostUpdate();
	}

	for (auto& obj : m_pGameObjects) {
		obj->PostUpdate();
	}
}

void Scene::GenerateSceneBound()
{
	for (const auto& pObj : m_pGameObjects) {
		const auto& pCollider = pObj->GetComponent<ICollider>();
		if (!pCollider) {
			continue;
		}

		const auto& xmAABB = pCollider->GetAABBFromOBBWorld();
		BoundingBox::CreateMerged(m_xmSceneBound, m_xmSceneBound, xmAABB);
	}
}

void Scene::CellPartition(const Vector2& v2OriginXZ, const Vector2& v2SizePerCellXZ, uint32 unCellsX, uint32 unCellsZ)
{
	// 1. Generate cells (2.5D)
	m_SpacePartition.v2SceneOriginXZ = v2OriginXZ;
	m_SpacePartition.v2CellSizeXZ = v2SizePerCellXZ;
	m_SpacePartition.xmui2NumCellsXZ = XMUINT2{ unCellsX, unCellsZ };
	m_SpacePartition.Cells.resize(unCellsX * unCellsZ);

	// 2. Check objects in cells
	for (const auto& pObj : m_pGameObjects) {
		m_SpacePartition.Insert(pObj);
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
		std::shared_ptr<IGameObject> pObj = std::make_shared<StaticObject>();
		pObj->SetName(jObject["ActorName"].get<std::string>());

		
		auto matrixData = jObject["Transform"]["WorldMatrix"].get<std::vector<float>>();
		Matrix M4x4WorldMatrix(matrixData.data());

		//Vector3 v3Rotation; // X : Pitch, Y : Yaw, Z : Roll
		//v3Rotation.x = jObject["Transform"]["Rotation"]["Pitch"].get<float>();
		//v3Rotation.y = jObject["Transform"]["Rotation"]["Yaw"].get<float>();
		//v3Rotation.z = jObject["Transform"]["Rotation"]["Roll"].get<float>();

		//Vector3 v3Scale;
		//v3Scale.x = jObject["Transform"]["Scale"]["X"].get<float>();
		//v3Scale.y = jObject["Transform"]["Scale"]["Y"].get<float>();
		//v3Scale.z = jObject["Transform"]["Scale"]["Z"].get<float>();

		pObj->GetTransform()->SetWorldMatrix(M4x4WorldMatrix);
		//pObj->GetTransform()->SetRotation(v3Rotation);
		//pObj->GetTransform()->Scale(v3Rotation);

		std::string strMeshName = jObject["MeshName"].get<std::string>();
		auto pMeshObject = MODEL->LoadOrGet(strMeshName)->CopyObject<NodeObject>();
		pObj->SetChild(pMeshObject);

		m_pGameObjects.push_back(pObj);
	}

}
