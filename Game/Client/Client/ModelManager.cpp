#include "pch.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "Skeleton.h"
#include "NodeObject.h"

void ModelManager::Initialize()
{
}

void ModelManager::LoadGameModels()
{
	LoadModelFromFile("Ch33_nonPBR");
	LoadModelFromFile("vintage_wooden_sniper_optimized_for_fpstps");
}

void ModelManager::Add(const std::string& strModelName, std::shared_ptr<IGameObject> pObj)
{
	if (!m_pModelPool.contains(strModelName)) {
		m_pModelPool.insert({ strModelName, pObj });
	}
}

std::shared_ptr<IGameObject> ModelManager::Get(const std::string& strObjName)
{
	auto it = m_pModelPool.find(strObjName);
	if (it == m_pModelPool.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<IGameObject> ModelManager::LoadOrGet(const std::string& strFileName, bool bUseNameFilenameOnRoot)
{
	auto it = m_pModelPool.find(strFileName);
	if (it == m_pModelPool.end()) {
		return LoadModelFromFile(strFileName, bUseNameFilenameOnRoot);
	}

	return it->second;
}

std::shared_ptr<IGameObject> ModelManager::LoadModelFromFile(const std::string& strFileName, bool bUseNameFilenameOnRoot)
{
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
	const auto& positions = inJson["Positions"];
	meshLoadInfo.v3Positions.resize(nVertices);
	for (size_t i = 0; i < nVertices; ++i) {
		const size_t base = i * 3;
		meshLoadInfo.v3Positions[i] = Vector3{
			positions[base + 0].get<float>(),
			positions[base + 1].get<float>(),
			positions[base + 2].get<float>()
		};
	}

	// Normals
	const auto& normals = inJson["Normals"];
	meshLoadInfo.v3Normals.resize(nVertices);
	for (size_t i = 0; i < nVertices; ++i) {
		const size_t base = i * 3;
		meshLoadInfo.v3Normals[i] = Vector3{
			normals[base + 0].get<float>(),
			normals[base + 1].get<float>(),
			normals[base + 2].get<float>()
		};
	}

	// Tangents
	const auto& tangents = inJson["Tangents"];
	meshLoadInfo.v3Tangents.resize(nVertices);
	for (size_t i = 0; i < nVertices; ++i) {
		const size_t base = i * 3;
		meshLoadInfo.v3Tangents[i] = Vector3{
			tangents[base + 0].get<float>(),
			tangents[base + 1].get<float>(),
			tangents[base + 2].get<float>()
		};
	}

	// TexCoord0
	unsigned nUVChannels = inJson["nUVChannels"].get<unsigned>();
	if (nUVChannels != 0) {
		const nlohmann::json& texCoordData = inJson["TexCoord0"];
		const auto& texCoord = texCoordData["TexCoord"];
		meshLoadInfo.v2TexCoord0.resize(nVertices);
		for (size_t i = 0; i < nVertices; ++i) {
			const size_t base = i * 2;
			meshLoadInfo.v2TexCoord0[i] = Vector2{
				texCoord[base + 0].get<float>(),
				texCoord[base + 1].get<float>(),
			};
		}
	}
	else {
		meshLoadInfo.v2TexCoord0.resize(nVertices);
	}

	meshLoadInfo.bIsSkinned = inJson["Skinned?"].get<bool>();
	if (meshLoadInfo.bIsSkinned) {
		// BlendIndices
		const auto& blendIndices = inJson["BlendIndices"];
		meshLoadInfo.xmun4BlendIndices.resize(nVertices);
		for (size_t i = 0; i < nVertices; ++i) {
			const size_t base = i * 4;
			meshLoadInfo.xmun4BlendIndices[i] = XMUINT4{
				blendIndices[base + 0].get<uint32>(),
				blendIndices[base + 1].get<uint32>(),
				blendIndices[base + 2].get<uint32>(),
				blendIndices[base + 3].get<uint32>()
			};
		}

		// BlendWeights
		const auto& blendWeights = inJson["BlendWeights"];
		meshLoadInfo.v4BlendWeights.resize(nVertices);
		for (size_t i = 0; i < nVertices; ++i) {
			const size_t base = i * 4;
			meshLoadInfo.v4BlendWeights[i] = Vector4{
				blendWeights[base + 0].get<float>(),
				blendWeights[base + 1].get<float>(),
				blendWeights[base + 2].get<float>(),
				blendWeights[base + 3].get<float>()
			};
		}

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

	return materialLoadInfo;
}

