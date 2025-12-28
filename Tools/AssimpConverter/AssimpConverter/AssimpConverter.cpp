#include "stdafx.h"
#include "AssimpConverter.h"

AssimpConverter::AssimpConverter()
{
	m_pImporter = std::make_shared<Assimp::Importer>();
}

AssimpConverter::~AssimpConverter()
{
}

void AssimpConverter::LoadFromFiles(const std::string& strPath)
{
	m_pScene = m_pImporter->ReadFile(
		strPath,
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		aiProcess_FlipWindingOrder |	// Convert to D3D
		//aiProcess_PreTransformVertices | 
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
		m_BoneIndexMap.insert({ aiString{ m_Bones[i].strFrameName }, i });
	}
	StoreBoneHeirachy(m_pRootNode);

	__debugbreak();
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
	b.xmf4x4Offset = aiMatrixToXMMatrix(pNode->mTransformation);
	m_Bones.push_back(b);

	for (int i = 0; i < pNode->mNumChildren; ++i) {
		StoreBoneData(pNode->mChildren[i]);
	}
}

void AssimpConverter::StoreBoneHeirachy(const aiNode* pNode)
{
	Bone& b = m_Bones[m_BoneIndexMap[pNode->mName]];

	if (pNode->mParent != nullptr) {
		b.nParentIdx = m_BoneIndexMap[pNode->mParent->mName];
	}

	b.nChildrenIdx.reserve(pNode->mNumChildren);
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		b.nChildrenIdx.push_back(m_BoneIndexMap[pNode->mChildren[i]->mName]);
	}
	
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		StoreBoneHeirachy(pNode->mChildren[i]);
	}
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

void AssimpConverter::ShowMeshData() const
{
	if (!m_pRootNode) {
		std::println("No Model Loaded");
	}

	ProcessMeshData(m_pRootNode, 0);
}

void AssimpConverter::ProcessMeshData(const aiNode* pNode, UINT nTabs) const
{
	for (int i = 0; i < nTabs; ++i) {
		std::print(".");
	}

	if (pNode->mNumMeshes == 0) {
		std::println("Node doesn't have mesh");
	}

	for (int meshIdx = 0; meshIdx < pNode->mNumMeshes; ++meshIdx) {
		aiMesh* pMesh = m_pScene->mMeshes[pNode->mMeshes[meshIdx]];

		aiVector3D v3AABBMax = pMesh->mAABB.mMax;
		aiVector3D v3AABBMin = pMesh->mAABB.mMin;

		// Positions
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3Positions = pMesh->mVertices[i];
		}

		// Normals
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3Positions = pMesh->mNormals[i];
		}

		// Tangents
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3Positions = pMesh->mTangents[i];
		}

		// Texture Coordinates (only [0], mostly main UV Channel)
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3TexCoord = pMesh->mTextureCoords[0][i];
		}

		// Skinned Data
		for (int i = 0; i < pMesh->mNumBones; ++i) {
			aiBone* pBone = pMesh->mBones[i];
			for (int j = 0; j < pBone->mNumWeights; ++j) {
				UINT vtxID = pBone->mWeights[j].mVertexId;
				float fWeight = pBone->mWeights[j].mWeight;
			}
		}

		for (int i = 0; i < pMesh->mNumFaces; ++i) {
			// Faces MUST have triangle primitives bc aiProcess_Triangulate is activated.
			aiFace face = pMesh->mFaces[i];
			for (int j = 0; j < face.mNumIndices; ++j) {
				UINT idx = face.mIndices[j];
			}
		}

		std::println("#{} | {} - numVertices : {}, numBones : {}, numFaces : {}",
			meshIdx, pMesh->mName.C_Str(), pMesh->mNumVertices, pMesh->mNumBones, pMesh->mNumFaces);
	}

	for (int i = 0; i < pNode->mNumChildren; ++i) {
		ProcessMeshData(pNode->mChildren[i], nTabs + 1);
	}


}

void AssimpConverter::ProcessMaterialData(const aiMesh* pMesh, UINT nTabs) const
{
	const aiMaterial* pMaterial = m_pScene->mMaterials[pMesh->mMaterialIndex];

	Material material;
	
	// Colors
	aiColor4D aicValue;
	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aicValue) == AI_SUCCESS) {
		material.xmf4AlbedoColor = aiColorToXMVector(aicValue);
	}
	
	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aicValue) == AI_SUCCESS) {
		material.xmf4AmbientColor = aiColorToXMVector(aicValue);
	}
	
	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		material.xmf4SpecularColor = aiColorToXMVector(aicValue);
	}
	
	if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aicValue) == AI_SUCCESS) {
		material.xmf4EmissiveColor = aiColorToXMVector(aicValue);
	}

	// Factors
	float fValue{};
	if (pMaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, fValue) == AI_SUCCESS) {
		material.fGlossiness = fValue;
	}

	if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, fValue) == AI_SUCCESS) {
		material.fSmoothness = 1 - fValue;
	}

	if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, fValue) == AI_SUCCESS) {
		material.fMetallic = fValue;
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		material.fSpecularHighlight = std::max(std::max(aicValue.r, aicValue.g), aicValue.b);
	}

	if (pMaterial->Get(AI_MATKEY_SHININESS, fValue) == AI_SUCCESS) {
		material.fSpecularHighlight = fValue;
	}

	// Textures
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

}

void AssimpConverter::ShowBoneData() const
{
}

void AssimpConverter::ProcessBoneData(const aiBone* pBone, UINT nTabs) const
{
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
			Bone bone = m_Bones[m_BoneIndexMap.find(pNodeAnim->mNodeName)->second];	// ???????

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