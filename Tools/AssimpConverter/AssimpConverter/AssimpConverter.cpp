#include "stdafx.h"
#include "AssimpConverter.h"

AssimpConverter::AssimpConverter()
{
	m_pImporter = std::make_shared<Assimp::Importer>();
}

AssimpConverter::~AssimpConverter()
{
}

void AssimpConverter::LoadFromFiles(const std::string& strPath, float fScaleFactor)
{
	m_fScale = fScaleFactor;
	m_strFilePath = strPath;

	m_pScene = m_pImporter->ReadFile(
		strPath,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |	// Convert to D3D
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_PopulateArmatureData | 
		aiProcess_LimitBoneWeights	// limits influence bone count to for per vertex
	);

	if (!m_pScene) {
		__debugbreak();
	}

	m_pRootNode = m_pScene->mRootNode;
	int nBones{};
	CountBones(m_pRootNode, nBones);
	m_Bones.reserve(nBones);
	
	StoreBoneData(m_pRootNode);
	for (int i = 0; i < nBones; ++i) {
		m_BoneIndexMap.insert({ m_Bones[i].strFrameName, i });
	}
	StoreBoneHeirachy(m_pRootNode);

}

void AssimpConverter::Serialize(const std::string& strPath, const std::string& strName) const
{
	namespace fs = std::filesystem;

	// 1. Make save path (if not exists)
	fs::path saveDirectoryPath{ strPath };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(strPath);
	}

	// 2. Defines Savefile name
	std::string strSave = std::format("{}/{}.json", strPath, strName);
	std::ofstream out(strSave);

	nlohmann::ordered_json HierarchyJson = StoreNodeToJson(m_pRootNode);
	out << HierarchyJson.dump(2);


	std::cout << "Serialization Complete at " << strSave << '\n';

}

void AssimpConverter::CountBones(const aiNode* pNode, int& nNumBones)
{
	nNumBones++;
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		CountBones(pNode->mChildren[i], nNumBones);
	}
}

void AssimpConverter::StoreBoneData(const aiNode* pNode)
{
	Bone b{};
	b.strFrameName = pNode->mName.C_Str();
	b.Transform = aiMatrixToXMMatrix(pNode->mTransformation);
	m_Bones.push_back(b);


	for (int i = 0; i < pNode->mNumChildren; ++i) {
		StoreBoneData(pNode->mChildren[i]);
	}
}

void AssimpConverter::StoreBoneHeirachy(const aiNode* pNode)
{
	Bone& b = m_Bones[m_BoneIndexMap[pNode->mName.C_Str()]];

	if (pNode->mParent != nullptr) {
		b.nParentIdx = m_BoneIndexMap[pNode->mParent->mName.C_Str()];
	}

	b.nChildrenIdx.reserve(pNode->mNumChildren);
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		b.nChildrenIdx.push_back(m_BoneIndexMap[std::string{ pNode->mChildren[i]->mName.C_Str() }]);
	}
	
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		StoreBoneHeirachy(pNode->mChildren[i]);
	}
}

nlohmann::ordered_json AssimpConverter::StoreNodeToJson(const aiNode* pNode) const
{
	nlohmann::ordered_json node;
	std::string strFrameName = pNode->mName.C_Str();
	XMFLOAT4X4 m = aiMatrixToXMMatrix(pNode->mTransformation);

	node["Name"] = strFrameName;
	node["Transform"] = {
		m._11, m._12, m._13, m._14,
		m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34,
		m._41, m._42, m._43, m._44,
	};

	// TODO : Mesh & Material
	node["nMeshes"] = pNode->mNumMeshes;
	if (pNode->mNumMeshes != 0) {
		for (int i = 0; i < pNode->mNumMeshes; ++i) {
			// Mesh
			std::string strMesh = "Mesh" + std::to_string(i);
			aiMesh* pMesh = m_pScene->mMeshes[pNode->mMeshes[i]];
			nlohmann::ordered_json mesh = StoreMeshToJson(pMesh);
			node[strMesh] = mesh;
		}
	}

	node["Children"] = nlohmann::ordered_json::array();

	for (int i = 0; i < pNode->mNumChildren; ++i) {
		nlohmann::ordered_json child = StoreNodeToJson(pNode->mChildren[i]);
		node["Children"].push_back(child);
	}

	return node;
}

nlohmann::ordered_json AssimpConverter::StoreMeshToJson(const aiMesh* pMesh) const
{
	nlohmann::ordered_json mesh;
	
	mesh["Name"] = std::string{ pMesh->mName.C_Str() };
	mesh["nVertices"] = pMesh->mNumVertices;

	// Position for create AABB
	std::vector<XMFLOAT3> xmf3Positions;
	xmf3Positions.reserve(pMesh->mNumVertices);

	// Positions
	mesh["Positions"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pMesh->mNumVertices; ++i) {
		aiVector3D v3Positions = pMesh->mVertices[i];
		XMFLOAT3 xmf3Position = aiVector3DToXMVector(v3Positions);
		
		// Scale if needs
		if (m_fScale != 1.f) {
			XMStoreFloat3(&xmf3Position, XMVectorScale(XMLoadFloat3(&xmf3Position), m_fScale));
		}

		mesh["Positions"].push_back(xmf3Position.x);
		mesh["Positions"].push_back(xmf3Position.y);
		mesh["Positions"].push_back(xmf3Position.z);

		xmf3Positions.push_back(xmf3Position);
	}

	mesh["nColorChannels"] = pMesh->GetNumColorChannels();
	for (int i = 0; i < pMesh->GetNumColorChannels(); ++i) {
		std::string strColor = "Color" + std::to_string(i);
		mesh[strColor] = nlohmann::ordered_json::array();
		for (int j = 0; j < pMesh->mNumVertices; ++j) {
			aiColor4D cColor = pMesh->mColors[i][j];
			mesh[strColor].push_back(cColor.r);
			mesh[strColor].push_back(cColor.g);
			mesh[strColor].push_back(cColor.b);
			mesh[strColor].push_back(cColor.a);
		}
	}

	// Normals
	mesh["Normals"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pMesh->mNumVertices; ++i) {
		aiVector3D v3Normals = pMesh->mNormals[i];
		mesh["Normals"].push_back(v3Normals.x);
		mesh["Normals"].push_back(v3Normals.y);
		mesh["Normals"].push_back(v3Normals.z);
	}

	// Tangents
	mesh["BiTangents"] = nlohmann::ordered_json::array();
	mesh["Tangents"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pMesh->mNumVertices; ++i) {
		aiVector3D v3Tangents = pMesh->mTangents[i];
		mesh["Tangents"].push_back(v3Tangents.x);
		mesh["Tangents"].push_back(v3Tangents.y);
		mesh["Tangents"].push_back(v3Tangents.z);
	}

	// BiTangents
	for (int i = 0; i < pMesh->mNumVertices; ++i) {
		aiVector3D v3BiTangents = pMesh->mBitangents[i];
		mesh["BiTangents"].push_back(v3BiTangents.x);
		mesh["BiTangents"].push_back(v3BiTangents.y);
		mesh["BiTangents"].push_back(v3BiTangents.z);
	}

	// Texture Coordinates (only [0], mostly main UV Channel)
	mesh["nUVChannels"] = pMesh->GetNumUVChannels();
	for (int i = 0; i < pMesh->GetNumUVChannels(); ++i) {
		std::string strTexCoord = "TexCoord" + std::to_string(i);
		aiVector3D* v3UVChannel = pMesh->mTextureCoords[i];
		mesh[strTexCoord]["nElements"] = pMesh->mNumUVComponents[i];
		mesh[strTexCoord]["TexCoord"] = nlohmann::ordered_json::array();
		for (int j = 0; j < pMesh->mNumVertices; ++j) {
			aiVector3D v3TexCoord = v3UVChannel[j];
			for (int k = 0; k < pMesh->mNumUVComponents[i]; ++k) {
				mesh[strTexCoord]["TexCoord"].push_back(v3TexCoord[k]);
			}
		}
	}

	// Skinned Data
	// TODO: Need check for mixamo animations (without skin)
	if (pMesh->mNumBones > 0) {
		mesh["Skinned?"] = true;
		mesh["BlendIndices"] = nlohmann::ordered_json::array();
		mesh["BlendWeights"] = nlohmann::ordered_json::array();

		// Gather skin data
		std::vector<SkinData> skinDatas(pMesh->mNumVertices);
		for (int boneIdx = 0; boneIdx < pMesh->mNumBones; ++boneIdx) {
			aiBone* pBone = pMesh->mBones[boneIdx];
			for (int weightIdx = 0; weightIdx < pBone->mNumWeights; ++weightIdx) {
				const aiVertexWeight& vertexWeight = pBone->mWeights[weightIdx];
				UINT vertexID = vertexWeight.mVertexId;
				float weight = vertexWeight.mWeight;
				for (int i = 0; i < 4; ++i) {
					if (skinDatas[vertexID].fBlendWeights[i] == 0.f) {
						skinDatas[vertexID].uiBlendIndices[i] = boneIdx;
						skinDatas[vertexID].fBlendWeights[i] = weight;
						break;
					}
				}
			}
		}

		// Normalize blend data and Serialize
		for (auto& v : skinDatas) {
			float fSum = std::accumulate(v.fBlendWeights.begin(), v.fBlendWeights.end(), 0);
			if (fSum > 0.f) {
				for (int i = 0; i < 4; ++i) {
					v.fBlendWeights[i] /= fSum;
				}
			}
			mesh["BlendIndices"].push_back(v.uiBlendIndices[0]);
			mesh["BlendIndices"].push_back(v.uiBlendIndices[1]);
			mesh["BlendIndices"].push_back(v.uiBlendIndices[2]);
			mesh["BlendIndices"].push_back(v.uiBlendIndices[3]);
			
			mesh["BlendWeights"].push_back(v.fBlendWeights[0]);
			mesh["BlendWeights"].push_back(v.fBlendWeights[1]);
			mesh["BlendWeights"].push_back(v.fBlendWeights[2]);
			mesh["BlendWeights"].push_back(v.fBlendWeights[3]);
		}
	}

	// Indices
	mesh["nIndices"] = pMesh->mNumFaces * 3;
	mesh["Indices"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pMesh->mNumFaces; ++i) {
		// Faces MUST have triangle primitives bc aiProcess_Triangulate is activated.
		aiFace face = pMesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; ++j) {
			UINT idx = face.mIndices[j];
			mesh["Indices"].push_back(idx);
		}
	}

	// Bounds (AABB)
	mesh["Bounds"]["Center"] = nlohmann::ordered_json::array();
	mesh["Bounds"]["Extents"] = nlohmann::ordered_json::array();

	BoundingBox xmAABB;
	BoundingBox::CreateFromPoints(xmAABB, xmf3Positions.size(), xmf3Positions.data(), sizeof(XMFLOAT3));

	mesh["Bounds"]["Center"].push_back(xmAABB.Center.x);
	mesh["Bounds"]["Center"].push_back(xmAABB.Center.y);
	mesh["Bounds"]["Center"].push_back(xmAABB.Center.z);

	mesh["Bounds"]["Extents"].push_back(xmAABB.Extents.x);
	mesh["Bounds"]["Extents"].push_back(xmAABB.Extents.y);
	mesh["Bounds"]["Extents"].push_back(xmAABB.Extents.z);

	nlohmann::ordered_json material = StoreMaterialToJson(m_pScene->mMaterials[pMesh->mMaterialIndex]);
	mesh["Material"] = material;


	return mesh;
}

nlohmann::ordered_json AssimpConverter::StoreMaterialToJson(const aiMaterial* pMaterial) const
{
	nlohmann::ordered_json material;

	// Colors
	aiColor4D aicValue;
	material["AlbedoColor"] = nlohmann::ordered_json::array();
	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aicValue) == AI_SUCCESS) {
		material["AlbedoColor"].push_back(aicValue.r);
		material["AlbedoColor"].push_back(aicValue.g);
		material["AlbedoColor"].push_back(aicValue.b);
		material["AlbedoColor"].push_back(aicValue.a);
	}
	else {
		material["AlbedoColor"] = { 0.f,0.f,0.f,1.f };
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aicValue) == AI_SUCCESS) {
		material["AmbientColor"] = nlohmann::ordered_json::array();
		material["AmbientColor"].push_back(aicValue.r);
		material["AmbientColor"].push_back(aicValue.g);
		material["AmbientColor"].push_back(aicValue.b);
		material["AmbientColor"].push_back(aicValue.a);
	}
	else {
		material["AmbientColor"] = { 0.f,0.f,0.f,1.f };
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		material["SpecularColor"] = nlohmann::ordered_json::array();
		material["SpecularColor"].push_back(aicValue.r);
		material["SpecularColor"].push_back(aicValue.g);
		material["SpecularColor"].push_back(aicValue.b);
		material["SpecularColor"].push_back(aicValue.a);
	}
	else {
		material["SpecularColor"] = { 1.f,1.f,1.f,1.f };
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aicValue) == AI_SUCCESS) {
		material["EmissiveColor"] = nlohmann::ordered_json::array();
		material["EmissiveColor"].push_back(aicValue.r);
		material["EmissiveColor"].push_back(aicValue.g);
		material["EmissiveColor"].push_back(aicValue.b);
		material["EmissiveColor"].push_back(aicValue.a);
	}
	else {
		material["EmissiveColor"] = { 0.f,0.f,0.f,1.f };
	}

	// Factors
	float fValue{};
	if (pMaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, fValue) == AI_SUCCESS) {
		material["fGlossiness"] = fValue;
	}

	if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, fValue) == AI_SUCCESS) {
		material["fSmoothness"] = 1 - fValue;;
	}

	if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, fValue) == AI_SUCCESS) {
		material["fMetallic"] = fValue;
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		material["fGlossyReflection"] = std::max(std::max(aicValue.r, aicValue.g), aicValue.b);
	}

	if (pMaterial->Get(AI_MATKEY_SHININESS, fValue) == AI_SUCCESS) {
		material["fSpecularHighlight"] = fValue;
	}

	// Textures
	// TODO : Make texture file from binary and serialize path
	std::vector<aiTextureType> textureTypes = {
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_METALNESS,
		aiTextureType_NORMALS,
	};

	aiString aistrTexturePath{};
	for (aiTextureType tType : textureTypes) {
		UINT nTextures = pMaterial->GetTextureCount(tType);
		for (int i = 0; i < nTextures; ++i) {
			if (pMaterial->GetTexture(tType, i, &aistrTexturePath) == AI_SUCCESS) {
				const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
				std::string strTextureFormat = pTexture->achFormatHint;

				// TODO : make it happen
				if (pTexture) {
					// Embedded texture
					if (pTexture->mHeight == 0) {
						// Compressed


					}
					else {
						// Raw RGBA


					}
				}
				else {
					// External texture

				}
			}
		}
	}

	return material;
}

void AssimpConverter::ShowFrameData() const
{
	if (!m_pRootNode) {
		std::println("No Model Loaded");
	}

	ProcessFrameData(m_pRootNode, 0);
}

void AssimpConverter::ProcessFrameData(const aiNode* pNode, UINT nTabs) const
{
	std::string strFrameName = pNode->mName.C_Str();

	// Print
	for (int i = 0; i < nTabs; ++i) {
		std::print(".");
	}

	std::println("{} : nMeshes : {}", strFrameName, pNode->mNumMeshes);

	for (int i = 0; i < pNode->mNumChildren; ++i) {
		ProcessFrameData(pNode->mChildren[i], nTabs + 1);
	}
}

void AssimpConverter::ProcessKeyframeData(const aiNodeAnim* pNode, UINT nTabs) const
{
	for (int i = 0; i < m_pScene->mNumAnimations; ++i) {
		aiAnimation* pAnim = m_pScene->mAnimations[i];
		for (int j = 0; j < pAnim->mNumChannels; ++j) {
			// These two below are NOT keyframe animation data
			//pAnim->mMeshChannels;
			//pAnim->mMorphMeshChannels;
			aiNodeAnim* pNodeAnim = pAnim->mChannels[j];

			for (int k = 0; k < pNodeAnim->mNumPositionKeys; ++k) {
				aiVectorKey Keyframe = pNodeAnim->mPositionKeys[k];
				double dTimeKey = Keyframe.mTime;
				aiVector3D v3Frame = Keyframe.mValue;
			}

			for (int k = 0; k < pNodeAnim->mNumRotationKeys; ++k) {
				aiQuatKey Keyframe = pNodeAnim->mRotationKeys[k];
				double dTimeKey = Keyframe.mTime;
				aiQuaternion v3Frame = Keyframe.mValue;
			}

			for (int k = 0; k < pNodeAnim->mNumScalingKeys; ++k) {
				aiVectorKey Keyframe = pNodeAnim->mScalingKeys[k];
				double dTimeKey = Keyframe.mTime;
				aiVector3D v3Frame = Keyframe.mValue;
			}
		}
	}
}