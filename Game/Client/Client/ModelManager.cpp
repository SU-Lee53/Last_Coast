#include "pch.h"
#include "ModelManager.h"
#include "AnimationManager.h"

void ModelManager::Initialize()
{
}

void ModelManager::LoadGameModels()
{
	LoadModelFromFile("Ch33_nonPBR");
	LoadModelFromFile("vintage_wooden_sniper_optimized_for_fpstps");
	LoadModelFromFile("Cube");
}

void ModelManager::Add(const std::string& strModelName, std::shared_ptr<GameObject> pObj)
{
	if (!m_pModelPool.contains(strModelName)) {
		m_pModelPool.insert({ strModelName, pObj });
	}
}

std::shared_ptr<GameObject> ModelManager::Get(const std::string& strObjName)
{
	auto it = m_pModelPool.find(strObjName);
	if (it == m_pModelPool.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<GameObject> ModelManager::LoadOrGet(const std::string& strFileName)
{
	auto it = m_pModelPool.find(strFileName);
	if (it == m_pModelPool.end()) {
		return LoadModelFromFile(strFileName);
	}

	return it->second;
}

std::shared_ptr<GameObject> ModelManager::LoadModelFromFile(const std::string& strFileName)
{
	std::string strFilePath = std::format("{}/{}.bin", g_strModelBasePath, strFileName);

	if (auto pObj = Get(strFileName)) {
		return pObj;
	}

	std::ifstream inFile{ strFilePath, std::ios::binary };
	if (!inFile) {
		__debugbreak();
		return nullptr;
	}

	std::vector<std::uint8_t> bson(std::istreambuf_iterator<char>(inFile), {}); 
	nlohmann::json j = nlohmann::json::from_bson(bson);;

	std::shared_ptr<GameObject> pGameObject;
	pGameObject = LoadFrameHierarchyFromFile(nullptr, j["Hierarchy"]);

	size_t nBones = j["nBones"].get<size_t>();
	if (nBones != 0) {
		std::vector<Bone> bones;
		bones.resize(nBones);
		for (const auto& jBone : j["Bones"]) {
			int nBoneIndex = jBone["Index"].get<int>();

			bones[nBoneIndex].nIndex = nBoneIndex;
			bones[nBoneIndex].nParentIndex = jBone["ParentIndex"].get<int>();
			bones[nBoneIndex].strBoneName = jBone["Name"].get<std::string>();

			bones[nBoneIndex].nDepth = jBone["Depth"].get<int>();
			bones[nBoneIndex].nChildren = jBone["nChildren"].get<int>();
			bones[nBoneIndex].nChilerenIndex.resize(bones[nBoneIndex].nChildren);
			std::vector<int> childrenIndex = jBone["Children"].get<std::vector<int>>();
			std::copy(childrenIndex.begin(), childrenIndex.end(), bones[nBoneIndex].nChilerenIndex.begin());

			bones[nBoneIndex].mtxTransform = Matrix(jBone["localBind"].get<std::vector<float>>().data());
			bones[nBoneIndex].mtxOffset = Matrix(jBone["inverseBind"].get<std::vector<float>>().data());

			if (bones[nBoneIndex].nParentIndex == -1) {
				pGameObject->m_nRootBoneIndex = bones[nBoneIndex].nIndex;
			}
		}
		pGameObject->m_Bones = bones;
	}

	if (pGameObject) {
		Add(strFileName, pGameObject);
	}

	return pGameObject;
}

std::shared_ptr<GameObject> ModelManager::LoadFrameHierarchyFromFile(std::shared_ptr<GameObject> pParent, const nlohmann::json& inJson)
{
	std::shared_ptr<GameObject> pGameObject = std::make_shared<GameObject>();

	unsigned nMeshes = inJson["nMeshes"].get<unsigned>();
	pGameObject->m_strFrameName = inJson["Name"].get<std::string>();
	pGameObject->GetTransform()->SetFrameMatrix(XMFLOAT4X4(inJson["Transform"].get<std::vector<float>>().data()));


	std::vector<MESHLOADINFO> meshLoadInfos;
	std::vector<MATERIALLOADINFO> materialLoadInfos;
	for (int i = 0; i < nMeshes; ++i) {
		auto [meshInfo, materialInfo] = LoadMeshInfoFromFiles(inJson["Meshes"][i]);
		meshLoadInfos.push_back(meshInfo);
		materialLoadInfos.push_back(materialInfo);
	}

	for (int i = 0; i < nMeshes; ++i) {
		pGameObject->AddComponent<MeshRenderer>(meshLoadInfos, materialLoadInfos);
	}
	
	if (pParent) {
		pGameObject->SetParent(pParent);
	}

	unsigned nChildren = inJson["nChildren"].get<unsigned>();
	pGameObject->m_pChildren.reserve(nChildren);
	for (int i = 0; i < nChildren; ++i) {
		pGameObject->m_pChildren.push_back(LoadFrameHierarchyFromFile(pGameObject, inJson["Children"][i]));
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
	std::vector<float> positions = inJson["Positions"].get<std::vector<float>>();
	meshLoadInfo.v3Positions.reserve(nVertices);
	std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.v3Positions), [&](size_t i) {
		size_t base = i * 3;
		return Vector3{ positions[base], positions[base + 1], positions[base + 2] };
	});

	// Normals
	std::vector<float> normals = inJson["Normals"].get<std::vector<float>>();
	meshLoadInfo.v3Normals.reserve(nVertices);
	std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.v3Normals), [&](size_t i) {
		size_t base = i * 3;
		return Vector3{ normals[base], normals[base + 1], normals[base + 2] };
	});

	// Tangents
	std::vector<float> tangents = inJson["Tangents"].get<std::vector<float>>();
	meshLoadInfo.v3Tangents.reserve(nVertices);
	std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.v3Tangents), [&](size_t i) {
		size_t base = i * 3;
		return Vector3{ tangents[base], tangents[base + 1], tangents[base + 2] };
	});

	// TexCoord0
	unsigned nUVChannels = inJson["nUVChannels"].get<unsigned>();
	if (nUVChannels != 0) {
		const nlohmann::json& texCoordData = inJson["TexCoord0"];
		std::vector<float> texCoord = texCoordData["TexCoord"].get<std::vector<float>>();
		meshLoadInfo.v2TexCoord0.reserve(nVertices);
		std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.v2TexCoord0), [&](size_t i) {
			size_t base = i * 2;
			return Vector2{ texCoord[base], texCoord[base + 1] };
			});
	}
	else {
		meshLoadInfo.v2TexCoord0.resize(nVertices);
	}

	meshLoadInfo.bIsSkinned = inJson["Skinned?"].get<bool>();
	if (meshLoadInfo.bIsSkinned) {
		// BlendIndices
		std::vector<int> blendIndices = inJson["BlendIndices"].get<std::vector<int>>();
		meshLoadInfo.xmun4BlendIndices.reserve(nVertices);
		std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.xmun4BlendIndices), [&](size_t i) {
			size_t base = i * 4;
			return XMUINT4{ (UINT)blendIndices[base], (UINT)blendIndices[base + 1], (UINT)blendIndices[base + 2], (UINT)blendIndices[base + 3] };
		});

		// BlendWeights
		std::vector<float> blendWeights = inJson["BlendWeights"].get<std::vector<float>>();
		meshLoadInfo.v4BlendWeights.reserve(nVertices);
		std::transform(loadIndices.begin(), loadIndices.end(), std::back_inserter(meshLoadInfo.v4BlendWeights), [&](size_t i) {
			size_t base = i * 4;
			return Vector4{ blendWeights[base], blendWeights[base + 1], blendWeights[base + 2], blendWeights[base + 3] };
		});

		meshLoadInfo.eMeshType = MESH_TYPE::SKINNED;
	}
	else {
		meshLoadInfo.xmun4BlendIndices.resize(nVertices);
		meshLoadInfo.v4BlendWeights.resize(nVertices);

		meshLoadInfo.eMeshType = MESH_TYPE::STATIC;
	}

	// Indices
	meshLoadInfo.unIndices = inJson["Indices"].get<std::vector<UINT>>();

	// Bounds (AABB)
	const nlohmann::json& aabbData = inJson["Bounds"];
	std::vector<float> fAABBCenter = aabbData["Center"].get<std::vector<float>>();
	meshLoadInfo.v3AABBCenter = Vector3(fAABBCenter[0], fAABBCenter[1], fAABBCenter[2]);
	std::vector<float> fAABBExtents = aabbData["Extents"].get<std::vector<float>>();
	meshLoadInfo.v3AABBExtents = Vector3(fAABBExtents[0], fAABBExtents[1], fAABBExtents[2]);

	// Material
	const nlohmann::json& materialData = inJson["Material"];
	materialLoadInfo = LoadMaterialInfoFromFiles(materialData[0]);


	return { meshLoadInfo, materialLoadInfo };
}

MATERIALLOADINFO ModelManager::LoadMaterialInfoFromFiles(const nlohmann::json& inJson)
{
	MATERIALLOADINFO materialLoadInfo;

	materialLoadInfo.v4Diffuse = Vector4(inJson["AlbedoColor"].get<std::vector<float>>().data());
	materialLoadInfo.v4Ambient = Vector4(inJson["AmbientColor"].get<std::vector<float>>().data());
	materialLoadInfo.v4Specular = Vector4(inJson["SpecularColor"].get<std::vector<float>>().data());
	materialLoadInfo.v4Emissive = Vector4(inJson["EmissiveColor"].get<std::vector<float>>().data());

	materialLoadInfo.fGlossiness = inJson["fGlossiness"].get<float>();
	materialLoadInfo.fSmoothness = inJson["fSmoothness"].get<float>();
	materialLoadInfo.fMetallic = inJson["fMetallic"].get<float>();
	materialLoadInfo.fGlossyReflection = inJson["fGlossyReflection"].get<float>();
	materialLoadInfo.fSpecularHighlight = inJson["fSpecularHighlight"].get<float>();

	materialLoadInfo.strAlbedoMapName = inJson["AlbedoMapName"].get<std::string>();
	materialLoadInfo.strSpecularMapName = inJson["SpecularMapName"].get<std::string>();
	materialLoadInfo.strMetallicMapName = inJson["MetallicMapName"].get<std::string>();
	materialLoadInfo.strNormalMapName = inJson["NormalMapName"].get<std::string>();

	return materialLoadInfo;
}

