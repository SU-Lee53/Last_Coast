#include "pch.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "Collider.h"
#include "Skeleton.h"
#include "NodeObject.h"

namespace {
	void ReadVector2ArrayFromJson(const nlohmann::json& j, uint32 unCount, OUT std::vector<Vector2>& outData)
	{
		outData.resize(unCount);
		for (uint32 i = 0; i < unCount; ++i) {
			const uint32 unBase = i * 2;
			outData[i] = Vector2{
				j[unBase + 0].get<float>(),
				j[unBase + 1].get<float>()
			};
		}
	}

	void ReadVector3ArrayFromJson(const nlohmann::json& j, uint32 unCount, OUT std::vector<Vector3>& outData)
	{
		outData.resize(unCount);
		for (uint32 i = 0; i < unCount; ++i) {
			const uint32 unBase = i * 3;
			outData[i] = Vector3{
				j[unBase + 0].get<float>(),
				j[unBase + 1].get<float>(),
				j[unBase + 2].get<float>()
			};
		}
	}

	void ReadVector4ArrayFromJson(const nlohmann::json& j, uint32 unCount, OUT std::vector<Vector4>& outData)
	{
		outData.resize(unCount);
		for (uint32 i = 0; i < unCount; ++i) {
			const uint32 unBase = i * 4;
			outData[i] = Vector4{
				j[unBase + 0].get<float>(),
				j[unBase + 1].get<float>(),
				j[unBase + 2].get<float>(),
				j[unBase + 3].get<float>()
			};
		}
	}

	void ReadXMUINT4ArrayFromJson(const nlohmann::json& j, uint32 unCount, OUT std::vector<XMUINT4>& outData)
	{
		outData.resize(unCount);
		for (uint32 i = 0; i < unCount; ++i) {
			const uint32 unBase = i * 4;
			outData[i] = XMUINT4{
				j[unBase + 0].get<uint32>(),
				j[unBase + 1].get<uint32>(),
				j[unBase + 2].get<uint32>(),
				j[unBase + 3].get<uint32>()
			};
		}
	}
}

void ModelManager::Initialize()
{
}

void ModelManager::LoadGameModels()
{
	std::lock_guard lock{ m_mtxModel };
	//LoadModelFromFile("Ch33_nonPBR");
	//LoadModelFromFile("player_f_02");
	//LoadModelFromFile("player_m_02");
	//LoadModelFromFile("vintage_wooden_sniper_optimized_for_fpstps");
}

void ModelManager::Add(const std::string& strModelName, std::shared_ptr<IGameObject> pObj)
{
	std::lock_guard lock{ m_mtxModel };
	if (!m_pModelPool.contains(strModelName)) {
		m_pModelPool.insert({ strModelName, pObj });
	}
}

std::shared_ptr<IGameObject> ModelManager::Get(const std::string& strObjName)
{
	std::lock_guard lock{ m_mtxModel };
	auto it = m_pModelPool.find(strObjName);
	if (it == m_pModelPool.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<IGameObject> ModelManager::LoadOrGet(const std::string& strFileName, bool bUseNameFilenameOnRoot)
{
	std::lock_guard lock{ m_mtxModel };
	auto it = m_pModelPool.find(strFileName);
	if (it == m_pModelPool.end()) {
		return LoadModelFromFile(strFileName, bUseNameFilenameOnRoot);
	}

	return it->second;
}

std::shared_ptr<IGameObject> ModelManager::LoadModelFromFile(const std::string& strFileName, bool bUseNameFilenameOnRoot)
{
	std::lock_guard lock{ m_mtxModel };
	if (auto pObj = Get(strFileName)) {
		return pObj;
	}

	std::string strFilePath = std::format("{}/{}.bin", g_strModelBasePath, strFileName);

	auto buf = ::ReadBinaryFile(strFilePath);
	nlohmann::json j = nlohmann::json::from_bson(buf);

	std::shared_ptr<IGameObject> pGameObject;
	int nIndex = 0;
	pGameObject = LoadFrameHierarchyFromFile(strFileName, nullptr, nullptr, j["Hierarchy"], bUseNameFilenameOnRoot, &nIndex);

	size_t nBones = j["nBones"].get<size_t>();
	if (nBones != 0) {
		std::vector<Bone> bones;
		bones.resize(nBones);
		for (const auto& jBone : j["Bones"]) {
			int nBoneIndex = jBone["Index"].get<int>();
			std::string strBoneName = jBone["Name"].get<std::string>();

			bones[nBoneIndex].nIndex = nBoneIndex;
			bones[nBoneIndex].nParentIndex = jBone["ParentIndex"].get<int>();
			bones[nBoneIndex].strBoneName = strBoneName;

			bones[nBoneIndex].nDepth = jBone["Depth"].get<int>();
			bones[nBoneIndex].nChildren = jBone["nChildren"].get<int>();
			bones[nBoneIndex].nChilerenIndex.resize(bones[nBoneIndex].nChildren);
			std::vector<int> childrenIndex = jBone["Children"].get<std::vector<int>>();
			std::copy(childrenIndex.begin(), childrenIndex.end(), bones[nBoneIndex].nChilerenIndex.begin());

			bones[nBoneIndex].mtxTransform = Matrix(jBone["localBind"].get<std::vector<float>>().data());
			bones[nBoneIndex].mtxOffset = Matrix(jBone["inverseBind"].get<std::vector<float>>().data());

			bones[nBoneIndex].pNode = pGameObject->FindFrameEndsWith(strBoneName);
		}
		pGameObject->AddComponent<Skeleton>(bones);
	}

	// 콜리전 정보를 NodeObject가 아닌 별도 풀에 저장
	// (StaticObject 루트에 붙여야 하므로 Scene::LoadFromFiles에서 꺼내 씀)
	if (j.contains("nCollisions") && j["nCollisions"].get<size_t>() > 0) {
		std::vector<COLLISIONMESHINFO> collisionInfos;
		collisionInfos.reserve(j["nCollisions"].get<size_t>());
		for (const auto& jCol : j["Collisions"]) {
			collisionInfos.push_back(LoadCollisionInfoFromJson(jCol));
		}

		if (!collisionInfos.empty()) {
			m_CollisionInfoPool[strFileName] = std::move(collisionInfos);
		}
	}
	else {
		COLLISIONMESHINFO collisionInfo = GatherRenderMeshCollisionInfo(j["Hierarchy"]);
		if (!collisionInfo.v3Positions.empty() && !collisionInfo.unIndices.empty()) {
			m_CollisionInfoPool[strFileName].push_back(std::move(collisionInfo));
		}
	}

	if (pGameObject) {
		Add(strFileName, pGameObject);
	}

	return pGameObject;
}

std::shared_ptr<IGameObject> ModelManager::LoadFrameHierarchyFromFile(const std::string& strFilename, std::shared_ptr<IGameObject> pParent, std::shared_ptr<IGameObject> pRoot, const nlohmann::json& inJson, bool bUseNameFilenameOnRoot, int32* poutnIndex)
{
	std::shared_ptr<IGameObject> pGameObject = std::make_shared<NodeObject>();

	unsigned nMeshes = inJson["nMeshes"].get<unsigned>();
	pGameObject->m_strFrameName = (bUseNameFilenameOnRoot) ? strFilename + std::to_string(*poutnIndex) : inJson["Name"].get<std::string>();
	pGameObject->GetTransform()->SetFrameMatrix(::ReadMatrixFromJson(inJson["Transform"]));
	

	std::vector<MESHLOADINFO> meshLoadInfos;
	std::vector<MATERIALLOADINFO> materialLoadInfos;
	meshLoadInfos.reserve(nMeshes);
	materialLoadInfos.reserve(nMeshes);
	for (int i = 0; i < nMeshes; ++i) {
		auto [meshInfo, materialInfo] = LoadMeshInfoFromFiles(inJson["Meshes"][i]);
		meshLoadInfos.push_back(std::move(meshInfo));
		materialLoadInfos.push_back(std::move(materialInfo));
	}

	if (meshLoadInfos.size() != 0) {
		pGameObject->AddComponent<MeshRenderer>(meshLoadInfos, materialLoadInfos);
	}

	if (pParent) {
		pGameObject->SetParent(pParent);
	}

	if (!pRoot) {
		pRoot = pGameObject;
	}
	else {
		pGameObject->SetRoot(pRoot);
	}

	unsigned nChildren = inJson["nChildren"].get<unsigned>();
	pGameObject->m_pChildren.reserve(nChildren);
	for (int i = 0; i < nChildren; ++i) {
		++(*poutnIndex);
		pGameObject->m_pChildren.push_back(LoadFrameHierarchyFromFile(strFilename, pGameObject, pRoot, inJson["Children"][i], false, poutnIndex));
	}

	return pGameObject;
}

std::pair<MESHLOADINFO, MATERIALLOADINFO> ModelManager::LoadMeshInfoFromFiles(const nlohmann::json& inJson)
{
	MESHLOADINFO meshLoadInfo;
	MATERIALLOADINFO materialLoadInfo;

	unsigned nVertices = 0;
	std::vector<size_t> loadIndices;
	nVertices = inJson["nVertices"].get<unsigned>();
	loadIndices.resize(nVertices);
	std::iota(loadIndices.begin(), loadIndices.end(), 0);

	// Positions
	ReadVector3ArrayFromJson(inJson["Positions"], nVertices, meshLoadInfo.v3Positions);

	// Normals
	ReadVector3ArrayFromJson(inJson["Normals"], nVertices, meshLoadInfo.v3Normals);

	// Tangents
	ReadVector3ArrayFromJson(inJson["Tangents"], nVertices, meshLoadInfo.v3Tangents);

	// TexCoord0
	unsigned nUVChannels = inJson["nUVChannels"].get<unsigned>();
	if (nUVChannels != 0) {
		const nlohmann::json& texCoordData = inJson["TexCoord0"];
		ReadVector2ArrayFromJson(texCoordData["TexCoord"], nVertices, meshLoadInfo.v2TexCoord0);
	}
	else {
		meshLoadInfo.v2TexCoord0.resize(nVertices);
	}

	meshLoadInfo.bIsSkinned = inJson["Skinned?"].get<bool>();
	if (meshLoadInfo.bIsSkinned) {
		// BlendIndices
		ReadXMUINT4ArrayFromJson(inJson["BlendIndices"], nVertices, meshLoadInfo.xmun4BlendIndices);

		// BlendWeights
		ReadVector4ArrayFromJson(inJson["BlendWeights"], nVertices, meshLoadInfo.v4BlendWeights);

		meshLoadInfo.eMeshType = MESH_TYPE::SKINNED;
	}
	else {
		meshLoadInfo.xmun4BlendIndices.resize(nVertices);
		meshLoadInfo.v4BlendWeights.resize(nVertices);

		meshLoadInfo.eMeshType = MESH_TYPE::STATIC;
	}

	// Indices
	inJson["Indices"].get_to(meshLoadInfo.unIndices);

	// Bounds (AABB)
	const nlohmann::json& aabbData = inJson["Bounds"];
	meshLoadInfo.v3AABBCenter = ::ReadVector3FromJson(aabbData["Center"]);
	meshLoadInfo.v3AABBExtents = ::ReadVector3FromJson(aabbData["Extents"]); 

	// Material
	const nlohmann::json& materialData = inJson["Material"];
	materialLoadInfo = LoadMaterialInfoFromFiles(materialData[0]);

	return { meshLoadInfo, materialLoadInfo };
}

MATERIALLOADINFO ModelManager::LoadMaterialInfoFromFiles(const nlohmann::json& inJson)
{
	MATERIALLOADINFO materialLoadInfo;

	materialLoadInfo.v4Diffuse = ::ReadVector4FromJson(inJson["AlbedoColor"]);
	materialLoadInfo.v4Ambient = ::ReadVector4FromJson(inJson["AmbientColor"]);
	materialLoadInfo.v4Specular = ::ReadVector4FromJson(inJson["SpecularColor"]);
	materialLoadInfo.v4Emissive = ::ReadVector4FromJson(inJson["EmissiveColor"]);

	materialLoadInfo.fGlossiness = inJson["fGlossiness"].get<float>();
	materialLoadInfo.fSmoothness = inJson["fSmoothness"].get<float>();
	materialLoadInfo.fMetallic = inJson["fMetallic"].get<float>();
	materialLoadInfo.fGlossyReflection = inJson["fGlossyReflection"].get<float>();
	materialLoadInfo.fSpecularHighlight = inJson["fSpecularHighlight"].get<float>();

	materialLoadInfo.strAlbedoMapName = inJson["AlbedoMapName"].get<std::string>();
	materialLoadInfo.strSpecularMapName = inJson["SpecularMapName"].get<std::string>();
	materialLoadInfo.strMetallicMapName = inJson["MetallicMapName"].get<std::string>();
	materialLoadInfo.strNormalMapName = inJson["NormalMapName"].get<std::string>();
	if (inJson.contains("AlbedoAlphaMode")) {
		materialLoadInfo.bHasAlbedoAlphaMode = true;
		materialLoadInfo.unAlbedoAlphaMode = inJson["AlbedoAlphaMode"].get<UINT>();
	}

	return materialLoadInfo;
}

const std::vector<COLLISIONMESHINFO>* ModelManager::GetCollisionInfos(const std::string& strModelName) const
{
	std::lock_guard lock{ m_mtxModel };
	auto it = m_CollisionInfoPool.find(strModelName);
	if (it == m_CollisionInfoPool.end())
		return nullptr;
	return &it->second;
}

COLLISIONMESHINFO ModelManager::GatherRenderMeshCollisionInfo(const nlohmann::json& nodeJson)
{
	COLLISIONMESHINFO info;
	info.strType = "RENDER";

	std::function<void(const nlohmann::json&)> Gather = [&](const nlohmann::json& node) {
		unsigned nMeshes = node["nMeshes"].get<unsigned>();
		for (unsigned i = 0; i < nMeshes; ++i) {
			const auto& meshJson = node["Meshes"][i];
			unsigned nVertices = meshJson["nVertices"].get<unsigned>();
			uint32 nBaseVertex = static_cast<uint32>(info.v3Positions.size());

			info.v3Positions.reserve(info.v3Positions.size() + nVertices);
			std::vector<Vector3> v3Positions;
			ReadVector3ArrayFromJson(meshJson["Positions"], nVertices, v3Positions);
			info.v3Positions.insert(info.v3Positions.end(), v3Positions.begin(), v3Positions.end());

			std::vector<uint32> indices;
			meshJson["Indices"].get_to(indices);
			info.unIndices.reserve(info.unIndices.size() + indices.size());
			for (uint32 idx : indices) {
				info.unIndices.push_back(nBaseVertex + idx);
			}
		}

		unsigned nChildren = node["nChildren"].get<unsigned>();
		for (unsigned i = 0; i < nChildren; ++i) {
			Gather(node["Children"][i]);
		}
	};

	Gather(nodeJson);
	return info;
}

COLLISIONMESHINFO ModelManager::LoadCollisionInfoFromJson(const nlohmann::json& inJson)
{
	COLLISIONMESHINFO info;
	info.strType = inJson["Type"].get<std::string>();
	info.strLinkedMesh = inJson["LinkedMesh"].get<std::string>();

	// Positions
	unsigned nVertices = inJson["nVertices"].get<unsigned>();
	ReadVector3ArrayFromJson(inJson["Positions"], nVertices, info.v3Positions);

	// Indices
	inJson["Indices"].get_to(info.unIndices);

	return info;
}

