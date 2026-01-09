#include "stdafx.h"
#include "AssimpConverter.h"

AssimpConverter::AssimpConverter()
{
	XMStoreFloat4x4(&m_xmf4x4SourceToEngine, XMMatrixIdentity());
	m_pImporter = std::make_shared<Assimp::Importer>();
}

void AssimpConverter::LoadFromFiles(const std::string& strPath, float fScaleFactor)
{
	m_fScale = fScaleFactor;
	m_strFilePath = strPath;

	m_Bones.clear();
	m_DFSBones.clear();
	m_BoneIndexRemappedDFS.clear();
	m_BoneIndexMap.clear();

	unsigned flags = 
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_PopulateArmatureData |
		aiProcess_LimitBoneWeights | 
		aiProcess_FlipUVs;

	m_pScene = m_pImporter->ReadFile(strPath, flags);

	if (m_pScene->mMetaData) {
		m_pScene->mMetaData->Get("UnitScaleFactor", m_dUnitScaleCM);
	}

	//m_pScene = m_pImporter->ReadFile(
	//	strPath,
	//	aiProcess_MakeLeftHanded |
	//	aiProcess_FlipUVs |
	//	aiProcess_FlipWindingOrder |	// Convert to D3D
	//	aiProcess_JoinIdenticalVertices |
	//	aiProcess_Triangulate |
	//	aiProcess_GenUVCoords |
	//	aiProcess_GenNormals |
	//	aiProcess_CalcTangentSpace |
	//	aiProcess_PopulateArmatureData | 
	//	aiProcess_LimitBoneWeights	// limits influencing bone count to for per vertex
	//);

	if (!m_pScene) {
		__debugbreak();
	}

	m_pRootNode = m_pScene->mRootNode;
	
	SceneAxis axisMetaData = ReadSceneAxisMetaData(m_pScene);
	bool bSourceWasRH = false;
	m_xmf4x4SourceToEngine = BuildSourceToEngineMatrix(axisMetaData, bSourceWasRH);
	m_bSourceWasRH = bSourceWasRH;

	GatherBoneIndex();
	BuildBoneHierarchy(m_pRootNode, -1);

	for (int i = 0; i < m_DFSBones.size(); ++i) {
		int nParentIndex = m_DFSBones[i].nParentIndex;
		if (nParentIndex != -1) {
			m_DFSBones[nParentIndex].nChildren++;
			m_DFSBones[nParentIndex].nChilerenIndex.push_back(i);
			m_DFSBones[i].nDepth = m_DFSBones[nParentIndex].nDepth + 1;
		}
	}
}

SceneAxis AssimpConverter::ReadSceneAxisMetaData(const aiScene* pScene)
{
	SceneAxis sceneAxis{};
	if (!pScene || !pScene->mMetaData) {
		return sceneAxis;
	}

	bool bHasMetaData = true;
	bHasMetaData &= pScene->mMetaData->Get("UpAxis", sceneAxis.nUpAxis);
	bHasMetaData &= pScene->mMetaData->Get("UpAxisSign", sceneAxis.nUpSign);

	bHasMetaData &= pScene->mMetaData->Get("FrontAxis", sceneAxis.nFrontAxis);
	bHasMetaData &= pScene->mMetaData->Get("FrontAxisSign", sceneAxis.nFrontSign);

	bHasMetaData &= pScene->mMetaData->Get("CoordAxis", sceneAxis.nCoordAxis);
	bHasMetaData &= pScene->mMetaData->Get("CoordAxisSign", sceneAxis.nCoordSign);

	sceneAxis.bHasMetadata = bHasMetaData;

	return sceneAxis;
}

XMFLOAT3 AssimpConverter::AxisToVector(int nAxisIndex, int nSign)
{
	float fSign = (nSign >= 0) ? 1.0f : -1.0f;
	switch (nAxisIndex) {
	case 0:
	{
		return XMFLOAT3(fSign, 0.f, 0.f);
	}
	case 1:
	{
		return XMFLOAT3(0.f, fSign, 0.f);
	}
	case 2:
	{
		return XMFLOAT3(0.f, 0.f, fSign);
	}
	default:
	{
		return XMFLOAT3(0.f, 0.f, 0.f);
	}
	}

	return XMFLOAT3(0.f, 0.f, 0.f);
}

Basis3 AssimpConverter::MakeSourceBasis(const SceneAxis& metaData)
{
	Basis3 basis{};
	if (!metaData.bHasMetadata) {
		basis.xmf3Right = XMFLOAT3(1.f, 0.f, 0.f);
		basis.xmf3Up = XMFLOAT3(0.f, 1.f, 0.f);
		basis.xmf3Forward = XMFLOAT3(0.f, 0.f, 1.f);
		return basis;
	}

	basis.xmf3Right = AxisToVector(metaData.nCoordAxis, metaData.nCoordSign);
	basis.xmf3Up = AxisToVector(metaData.nUpAxis, metaData.nUpSign);
	basis.xmf3Forward = AxisToVector(metaData.nFrontAxis, metaData.nFrontSign);

	return basis;
}

bool AssimpConverter::IsRightHanded(const Basis3& basis)
{
	XMVECTOR xmvCross = XMVector3Cross(XMLoadFloat3(&basis.xmf3Right), XMLoadFloat3(&basis.xmf3Up));
	float fCheck = XMVectorGetX(XMVector3Dot(xmvCross, XMLoadFloat3(&basis.xmf3Forward)));

	return fCheck > 0.f;
}

void AssimpConverter::ApplyReflectionRH(Basis3& basis, bool& outbSceneWasRH)
{
	outbSceneWasRH = IsRightHanded(basis);
	if (outbSceneWasRH) {
		XMVECTOR xmvNegate = XMVectorNegate(XMLoadFloat3(&basis.xmf3Forward));
		XMStoreFloat3(&basis.xmf3Forward, xmvNegate);
	}
}

XMFLOAT4X4 AssimpConverter::MakeBasisMatrix(const Basis3& basis)
{
	return XMFLOAT4X4(
		basis.xmf3Right.x,		basis.xmf3Right.y,		basis.xmf3Right.z,		0.f,
		basis.xmf3Up.x,			basis.xmf3Up.y,			basis.xmf3Up.z,			0.f,
		basis.xmf3Forward.x,	basis.xmf3Forward.y,	basis.xmf3Forward.z,	0.f,
		0.f,					0.f,					0.f,					1.f
	);
}

XMFLOAT4X4 AssimpConverter::BuildSourceToEngineMatrix(const SceneAxis& metaData, bool& outbWasRH)
{
	Basis3 sourceBasis = MakeSourceBasis(metaData);
	ApplyReflectionRH(sourceBasis, outbWasRH);
	XMFLOAT4X4 xmf4x4SrcToEngine = MakeBasisMatrix(sourceBasis);
	
	if (m_bForceBakeForwardZ) {
		XMMATRIX xmmtxSrcToEngine = XMLoadFloat4x4(&xmf4x4SrcToEngine);
		xmmtxSrcToEngine = XMMatrixMultiply(xmmtxSrcToEngine, XMMatrixRotationY(-XM_PIDIV2));
		XMStoreFloat4x4(&xmf4x4SrcToEngine, xmmtxSrcToEngine);
	}

	return xmf4x4SrcToEngine;
}

std::string NormalizeBoneName(const std::string& name)
{
	auto pos = name.find(':');
	if (pos != std::string::npos)
		return name.substr(pos + 1);
	return name;
}

void AssimpConverter::GatherBoneIndex()
{
	for (int m = 0; m < m_pScene->mNumMeshes; ++m)
	{
		const aiMesh* pMesh = m_pScene->mMeshes[m];

		for (int b = 0; b < pMesh->mNumBones; ++b)
		{
			const aiBone* pBone = pMesh->mBones[b];
			std::string strName = NormalizeBoneName(pBone->mName.C_Str());

			if (m_BoneIndexMap.find(strName) == m_BoneIndexMap.end())
			{
				int index = m_Bones.size();
				m_BoneIndexMap[strName] = index;

				Bone info{};
				info.nIndex = index;
				info.strName = strName;
				info.xmf4x4Offset = aiMatrixToXMMatrix(pBone->mOffsetMatrix);
				info.nParentIndex = -1;	// Later

				m_Bones.push_back(info);
			}
		}
	}
}

void AssimpConverter::BuildBoneHierarchy(aiNode* node, int parentBoneIndex)
{
	std::string strNodeName = NormalizeBoneName(node->mName.C_Str());
	int newBoneIndex = -1;

	auto it = m_BoneIndexMap.find(strNodeName);
	if (it != m_BoneIndexMap.end())
	{
		int oldBoneIndex = it->second;

		newBoneIndex = (int)m_DFSBones.size();
		m_BoneIndexRemappedDFS[oldBoneIndex] = newBoneIndex;

		Bone info{};
		info.nIndex = newBoneIndex;
		info.strName = strNodeName;
		info.nParentIndex = parentBoneIndex;
		info.xmf4x4Transform = aiMatrixToXMMatrix(node->mTransformation);
		info.xmf4x4Offset = m_Bones[oldBoneIndex].xmf4x4Offset;

		float fScale = GetFinalScale();

		info.xmf4x4Transform._41 *= fScale;
		info.xmf4x4Transform._42 *= fScale;
		info.xmf4x4Transform._43 *= fScale;

		info.xmf4x4Offset._41 *= fScale;
		info.xmf4x4Offset._42 *= fScale;
		info.xmf4x4Offset._43 *= fScale;

		m_DFSBones.push_back(info);

		//currentBoneIndex = it->second;
		//m_Bones[currentBoneIndex].nParentIndex = parentBoneIndex;
		//m_Bones[currentBoneIndex].xmf4x4Transform = aiMatrixToXMMatrix(node->mTransformation);
	}

	for (int i = 0; i < node->mNumChildren; ++i)
	{
		BuildBoneHierarchy(
			node->mChildren[i],
			newBoneIndex == -1 ? parentBoneIndex : newBoneIndex
		);
	}
}

void AssimpConverter::SerializeModel(const std::string& strPath, const std::string& strName)
{
	namespace fs = std::filesystem;
	m_strSavePath = strPath + '\\' + strName;


	std::string strSave = std::format("{}\\Models\\", m_strSavePath);
	fs::path saveDirectoryPath{ strSave };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(strSave);
	}

	DisplayText("Serializing...\n");

	nlohmann::ordered_json hierarchyJson;
	hierarchyJson["Hierarchy"] = StoreNodeToJson(m_pRootNode);

	// Bone data (For animation retargeting)
	hierarchyJson["nBones"] = m_DFSBones.size();
	hierarchyJson["Bones"] = nlohmann::ordered_json::array();
	for (const auto& boneData : m_DFSBones) {
		nlohmann::ordered_json bone;
		bone["Name"] = boneData.strName;
		bone["Index"] = boneData.nIndex;
		bone["ParentIndex"] = boneData.nParentIndex;
		bone["Depth"] = boneData.nDepth;

		int nChildren = boneData.nChildren;
		bone["nChildren"] = nChildren;
		bone["Children"] = nlohmann::ordered_json::array();
		for (int i = 0; i < nChildren; ++i) {
			bone["Children"].push_back(boneData.nChilerenIndex[i]);
		}

		XMFLOAT4X4 m = ConvertMatrixToEngine(boneData.xmf4x4Transform);
		bone["localBind"] = {
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44,
		};

		m = ConvertMatrixToEngine(boneData.xmf4x4Offset);
		bone["inverseBind"] = {
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44,
		};

		hierarchyJson["Bones"].push_back(bone);
	}

	strSave = std::format("{}\\Models\\{}.bin", m_strSavePath, strName);
	std::ofstream out{ strSave, std::ios::binary };

	//out << hierarchyJson.dump(2);

	std::vector<uint8_t> bson = nlohmann::json::to_bson(hierarchyJson);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());

	DisplayText("Successfully serialized at %s\r\n", m_strSavePath.c_str());

}

nlohmann::ordered_json AssimpConverter::StoreNodeToJson(const aiNode* pNode) const
{
	nlohmann::ordered_json node;
	std::string strFrameName = pNode->mName.C_Str();
	XMFLOAT4X4 xmf4x4Transform = aiMatrixToXMMatrix(pNode->mTransformation);
	XMFLOAT4X4 m = ConvertMatrixToEngine(xmf4x4Transform);

	node["Name"] = strFrameName;
	node["Transform"] = {
		m._11, m._12, m._13, m._14,
		m._21, m._22, m._23, m._24,
		m._31, m._32, m._33, m._34,
		m._41, m._42, m._43, m._44,
	};

	// Mesh & Material
	node["nMeshes"] = pNode->mNumMeshes;
	node["Meshes"] = nlohmann::ordered_json::array();
	if (pNode->mNumMeshes != 0) {
		for (int i = 0; i < pNode->mNumMeshes; ++i) {
			aiMesh* pMesh = m_pScene->mMeshes[pNode->mMeshes[i]];
			nlohmann::ordered_json mesh = StoreMeshToJson(pMesh);
			node["Meshes"].push_back(mesh);
		}
	}

	// Children
	node["nChildren"] = pNode->mNumChildren;
	node["Children"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pNode->mNumChildren; ++i) {
		nlohmann::ordered_json child = StoreNodeToJson(pNode->mChildren[i]);
		node["Children"].push_back(child);
	}

	return node;
}

nlohmann::ordered_json AssimpConverter::StoreMeshToJson(const aiMesh* pMesh) const
{
	XMMATRIX xmmtxSceneToEngine = XMLoadFloat4x4(&m_xmf4x4SourceToEngine);
	float fScale = GetFinalScale();
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
		//if (pMesh->mNumBones <= 0 && m_fScale != 1.f) {
		//	XMStoreFloat3(&xmf3Position, XMVectorScale(XMLoadFloat3(&xmf3Position), m_fScale));
		//}
		XMStoreFloat3(&xmf3Position, XMVectorScale(XMLoadFloat3(&xmf3Position), fScale));



		XMVECTOR xmvPosition = XMLoadFloat3(&xmf3Position);
		xmvPosition = XMVector3TransformCoord(xmvPosition, xmmtxSceneToEngine);
		XMStoreFloat3(&xmf3Position, xmvPosition);

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
		XMFLOAT3 xmf3Normals = aiVector3DToXMVector(v3Normals);

		XMVECTOR xmvNormal = XMLoadFloat3(&xmf3Normals);
		xmvNormal = XMVector3TransformNormal(xmvNormal, xmmtxSceneToEngine);
		xmvNormal = XMVector3Normalize(xmvNormal);
		XMStoreFloat3(&xmf3Normals, xmvNormal);

		mesh["Normals"].push_back(xmf3Normals.x);
		mesh["Normals"].push_back(xmf3Normals.y);
		mesh["Normals"].push_back(xmf3Normals.z);
	}

	// Tangents
	mesh["Tangents"] = nlohmann::ordered_json::array();
	if (pMesh->mTangents) {
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3Tangents = pMesh->mTangents[i];
			XMFLOAT3 xmf3Tangents = aiVector3DToXMVector(v3Tangents);

			XMVECTOR xmvTangent = XMLoadFloat3(&xmf3Tangents);
			xmvTangent = XMVector3TransformNormal(xmvTangent, xmmtxSceneToEngine);
			xmvTangent = XMVector3Normalize(xmvTangent);
			XMStoreFloat3(&xmf3Tangents, xmvTangent);

			mesh["Tangents"].push_back(xmf3Tangents.x);
			mesh["Tangents"].push_back(xmf3Tangents.y);
			mesh["Tangents"].push_back(xmf3Tangents.z);
		}
	}
	else {
		for (int i = 0; i < pMesh->mNumVertices * 3; ++i) {
			mesh["Tangents"].push_back(0.f);
		}
	}

	// BiTangents
	mesh["BiTangents"] = nlohmann::ordered_json::array();
	if (pMesh->mBitangents) {
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3BiTangents = pMesh->mBitangents[i];
			XMFLOAT3 xmf3BiTangents = aiVector3DToXMVector(v3BiTangents);

			XMVECTOR xmvBiTangents = XMLoadFloat3(&xmf3BiTangents);
			xmvBiTangents = XMVector3TransformNormal(xmvBiTangents, xmmtxSceneToEngine);
			xmvBiTangents = XMVector3Normalize(xmvBiTangents);
			XMStoreFloat3(&xmf3BiTangents, xmvBiTangents);

			mesh["BiTangents"].push_back(xmf3BiTangents.x);
			mesh["BiTangents"].push_back(xmf3BiTangents.y);
			mesh["BiTangents"].push_back(xmf3BiTangents.z);
		}
	}
	else {
		for (int i = 0; i < pMesh->mNumVertices * 3; ++i) {
			mesh["BiTangents"].push_back(0.f);
		}
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
			const aiBone* pBone = pMesh->mBones[boneIdx];

			int oldBoneIndex = m_BoneIndexMap.at(NormalizeBoneName(pBone->mName.C_Str()));
			int nDFSIndex = m_BoneIndexRemappedDFS.at(oldBoneIndex);

			for (int weightIdx = 0; weightIdx < pBone->mNumWeights; ++weightIdx) {
				const aiVertexWeight& vertexWeight = pBone->mWeights[weightIdx];
				UINT vertexID = vertexWeight.mVertexId;
				float weight = vertexWeight.mWeight;

				for (int i = 0; i < 4; ++i) {
					if (skinDatas[vertexID].fBlendWeights[i] == 0.f) {
						skinDatas[vertexID].uiBlendIndices[i] = nDFSIndex;
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
	else {
		mesh["Skinned?"] = false;
		mesh["BlendIndices"] = nullptr;
		mesh["BlendWeights"] = nullptr;
	}

	// Indices
	mesh["nIndices"] = pMesh->mNumFaces * 3;
	mesh["Indices"] = nlohmann::ordered_json::array();
	std::vector<UINT> faces(3);
	for (int i = 0; i < pMesh->mNumFaces; ++i) {
		// Faces MUST have triangle primitives bc aiProcess_Triangulate is activated.
		aiFace face = pMesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; ++j) {
			faces[j] = face.mIndices[j];
		}

		if (m_bSourceWasRH) {
			std::swap(faces[1], faces[2]);
		}

		for (int j = 0; j < face.mNumIndices; ++j) {
			mesh["Indices"].push_back(faces[j]);
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
	mesh["Material"].push_back(material);


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
	else {
		material["fGlossiness"] = 0.f;
	}

	if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, fValue) == AI_SUCCESS) {
		material["fSmoothness"] = 1 - fValue;;
	}
	else {
		material["fSmoothness"] = 0.f;
	}

	if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, fValue) == AI_SUCCESS) {
		material["fMetallic"] = fValue;
	}
	else {
		material["fMetallic"] = 0.f;
	}

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aicValue) == AI_SUCCESS) {
		material["fGlossyReflection"] = std::max(std::max(aicValue.r, aicValue.g), aicValue.b);
	}
	else {
		material["fGlossyReflection"] = 0.f;
	}

	if (pMaterial->Get(AI_MATKEY_SHININESS, fValue) == AI_SUCCESS) {
		material["fSpecularHighlight"] = fValue;
	}
	else {
		material["fSpecularHighlight"] = 0.f;
	}

	// Textures
	// TODO : Make texture file from binary and serialize path
	material["AlbedoMapName"] = "None";
	material["SpecularMapName"] = "None";
	material["MetallicMapName"] = "None";
	material["NormalMapName"] = "None";

	std::vector<aiTextureType> etextureTypes = {
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_METALNESS,
		aiTextureType_NORMALS,
		aiTextureType_EMISSIVE,
	};

	for (aiTextureType eType : etextureTypes) {
		UINT nTextures = pMaterial->GetTextureCount(eType);
		for (int i = 0; i < nTextures; ++i) {
			aiString aistrTexturePath{};
			if (pMaterial->GetTexture(eType, i, &aistrTexturePath) == AI_SUCCESS) {
				const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(aistrTexturePath.C_Str());
				// TODO : make it happen
				std::string strTextureName;
				if (pTexture) {
					ExportEmbeddedTexture(pTexture, eType);
					strTextureName = pTexture->mFilename.C_Str();
					strTextureName = std::filesystem::path{ strTextureName }.stem().string();
				}
				else {
					// External texture
					ExportExternalTexture(aistrTexturePath, eType);
					strTextureName = aistrTexturePath.C_Str();
					strTextureName = std::filesystem::path{ strTextureName }.stem().string();
				}

				switch (eType) {
				case aiTextureType_DIFFUSE:
				{
					material["AlbedoMapName"] = strTextureName;
					break;
				}
				case aiTextureType_SPECULAR:
				{
					material["SpecularMapName"] = strTextureName;
					break;
				}
				case aiTextureType_METALNESS:
				{
					material["MetallicMapName"] = strTextureName;
					break;
				}
				case aiTextureType_NORMALS:
				{
					material["NormalMapName"] = strTextureName;
					break;
				}
				case aiTextureType_EMISSIVE:
				{
					material["EmissionMapName"] = strTextureName;
					break;
				}
				default:
					std::unreachable();
				}
			}
		}
	}

	return material;
}

void AssimpConverter::ExportEmbeddedTexture(const aiTexture* pTexture, aiTextureType eTextureType) const
{
	namespace fs = std::filesystem;

	HRESULT hr{};
	ScratchImage img{};
	TexMetadata metaData{};
	std::string strTextureFormat = pTexture->achFormatHint;

	// 1. Make save path (if not exists)
	std::string strTexturePath = std::format("{}\\Textures", m_strSavePath);
	fs::path saveDirectoryPath{ strTexturePath };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(strTexturePath);
	}

	// 2. Get Texture name
	std::string strTextureName = pTexture->mFilename.C_Str();
	strTextureName = fs::path{ strTextureName }.stem().string();

	std::string strTextureSaveName = std::format("{}\\{}.dds", strTexturePath, strTextureName);
	if (fs::exists(fs::path{ strTextureSaveName })) {
		DisplayText("Texture : %s is already exists\n", strTextureSaveName.c_str());
		return;
	}

	if (pTexture->mHeight == 0) {
		// Compressed
		if (IsDDS(pTexture)) {
			std::ofstream out{ strTextureSaveName , std::ios::binary };
			out.write(
				reinterpret_cast<const char*>(pTexture->pcData),
				pTexture->mWidth
			);

			return;
		}
		else {
			hr = ::LoadFromWICMemory(
				reinterpret_cast<const uint8_t*>(pTexture->pcData),
				pTexture->mWidth,
				WIC_FLAGS_NONE,
				&metaData,
				img
			);

			// Filp Y if its normal map
			if (eTextureType == aiTextureType_NORMALS) {
				FlipNormalMapY(img);
			}

			if (FAILED(hr)) {
				DisplayText("Failed to load texutre");
				return;
			}
		}
	}
	else {
		// Raw RGBA
		hr = img.Initialize2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			pTexture->mWidth,
			pTexture->mHeight,
			1,
			1
		);

		// Filp Y if its normal map
		if (eTextureType == aiTextureType_NORMALS) {
			FlipNormalMapY(img);
		}

		if (FAILED(hr)) {
			DisplayText("Failed to load texutre");
			return;
		}

		::memcpy(img.GetImage(0, 0, 0)->pixels, pTexture->pcData, (pTexture->mWidth * pTexture->mHeight * 4));
	}

	// Generate Mipmaps
	ScratchImage mipChain{};

	HRESULT hrMipGenerated = ::GenerateMipMaps(
		img.GetImages(),
		img.GetImageCount(),
		img.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,	// auto
		mipChain
	);

	// Save
	hr = ::SaveToDDSFile(
		hrMipGenerated == S_OK ? mipChain.GetImages() : img.GetImages(),
		hrMipGenerated == S_OK ? mipChain.GetImageCount() : img.GetImageCount(),
		hrMipGenerated == S_OK ? mipChain.GetMetadata() : img.GetMetadata(),
		DirectX::DDS_FLAGS_NONE,
		StringToWString_UTF8(strTextureSaveName).c_str()
	);

	if (FAILED(hr)) {
		DisplayText("Failed to save texutre");
	}

	return;
}

void AssimpConverter::ExportExternalTexture(const aiString& aistrTexturePath, aiTextureType eTextureType) const
{
	namespace fs = std::filesystem;
	fs::path texFullPath = fs::path{ m_strFilePath }.parent_path() / aistrTexturePath.C_Str();

	// 1. Make save path (if not exists)
	std::string strTexturePath = std::format("{}\\Textures", m_strSavePath);
	fs::path saveDirectoryPath{ strTexturePath };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(strTexturePath);
	}

	// 2. Get Texture name
	std::string strTextureName = fs::path{ texFullPath }.stem().string();
	std::string strTextureSaveName = std::format("{}\\{}.dds", strTexturePath, strTextureName);
	if (fs::exists(fs::path{ strTextureSaveName })) {
		DisplayText("Texture : %s is already exists\n", strTextureSaveName.c_str());
		return;
	}

	ScratchImage img{};
	TexMetadata metadata{};

	HRESULT hr = LoadFromWICFile(
		texFullPath.c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		img
	);

	if (FAILED(hr)) {
		DisplayText("Failed to load texutre");
	}

	// Filp Y if its normal map
	if (eTextureType == aiTextureType_NORMALS) {
		FlipNormalMapY(img);
	}

	// Generate Mipmaps
	ScratchImage mipChain{};

	HRESULT hrMipGenerated = ::GenerateMipMaps(
		img.GetImages(),
		img.GetImageCount(),
		img.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,	// auto
		mipChain
	);

	// Save
	hr = ::SaveToDDSFile(
		hrMipGenerated == S_OK ? mipChain.GetImages() : img.GetImages(),
		hrMipGenerated == S_OK ? mipChain.GetImageCount() : img.GetImageCount(),
		hrMipGenerated == S_OK ? mipChain.GetMetadata() : img.GetMetadata(),
		DirectX::DDS_FLAGS_NONE,
		StringToWString_UTF8(strTextureSaveName).c_str()
	);

	if (FAILED(hr)) {
		DisplayText("Failed to save texutre");
	}

	return;
}

void AssimpConverter::SerializeAnimation(const std::string& strPath, const std::string& strName)
{
	if (m_pScene->mNumAnimations == 0) {
		DisplayText("File doesn't have keyframe animation");
	}

	namespace fs = std::filesystem;
	m_strSavePath = strPath + '\\' + strName;

	// Make save path (if not exists)
	fs::path saveDirectoryPath{ m_strSavePath };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(m_strSavePath);
	}

	DisplayText("Serializing...\n");

	nlohmann::ordered_json animJson;
	animJson["nAnimations"] = m_pScene->mNumAnimations;
	animJson["Animations"] = nlohmann::ordered_json::array();
	for (int i = 0; i < m_pScene->mNumAnimations; ++i) {
		animJson["Animations"].push_back(StoreAnimationToJson(m_pScene->mAnimations[i]));
	}

	//std::ofstream out(strSave);
	//out << animJson.dump(2);

	std::string strSave = std::format("{}/{}.bin", m_strSavePath, strName);
	std::ofstream out{ strSave, std::ios::binary };

	std::vector<uint8_t> bson = nlohmann::json::to_bson(animJson);
	out.write(reinterpret_cast<const char*>(bson.data()), bson.size());

	DisplayText("Successfully serialized at %s\r\n", m_strSavePath.c_str());
}

nlohmann::ordered_json AssimpConverter::StoreAnimationToJson(const aiAnimation* pAnimation) const
{
	// These two below are NOT keyframe animation data
	//	pAnimation->mMeshChannels; -> Tweening
	//	pAnimation->mMorphMeshChannels; -> Morphing
	nlohmann::ordered_json anim;
	double dTicksPerSecond = pAnimation->mTicksPerSecond;
	if (pAnimation->mTicksPerSecond == 0) {
		dTicksPerSecond = 30.0;
	}

	anim["Name"] = NormalizeBoneName(pAnimation->mName.C_Str());
	anim["Duration"] = pAnimation->mDuration / dTicksPerSecond;
	anim["TicksPerSecond"] = dTicksPerSecond;

	anim["nChannels"] = pAnimation->mNumChannels;
	anim["Channels"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pAnimation->mNumChannels; ++i) {
		anim["Channels"].push_back(StoreNodeAnimToJson(pAnimation->mChannels[i], dTicksPerSecond));
	}

	return anim;
}

nlohmann::ordered_json AssimpConverter::StoreNodeAnimToJson(const aiNodeAnim* pNodeAnim, double dTicksPerSecond) const
{
	nlohmann::ordered_json nodeAnim;
	nodeAnim["Name"] = NormalizeBoneName(pNodeAnim->mNodeName.C_Str());	// name of bone

	std::map<double, KeyFrame> keyFrameDatas;

	std::set<double> keys;
	for (int i = 0; i < pNodeAnim->mNumPositionKeys; ++i) {
		aiVectorKey keyFrame = pNodeAnim->mPositionKeys[i];
		keys.insert(keyFrame.mTime);
	}
	
	for (int i = 0; i < pNodeAnim->mNumRotationKeys; ++i) {
		aiQuatKey keyFrame = pNodeAnim->mRotationKeys[i];
		keys.insert(keyFrame.mTime);
	}
	
	for (int i = 0; i < pNodeAnim->mNumScalingKeys; ++i) {
		aiVectorKey keyFrame = pNodeAnim->mScalingKeys[i];
		keys.insert(keyFrame.mTime);
	}

	XMFLOAT4 xmf4PrevQuaternion = { 0,0,0,1 };
	bool bHasPrevQuaternion = false;
	for (double timeKey : keys)
	{
		KeyFrame keyFrame;
		keyFrame.xmf3Position = SamplePosition(pNodeAnim, timeKey);
		keyFrame.xmf4RotationQuat = SampleRotation(pNodeAnim, timeKey);
		keyFrame.xmf3Scale = SampleScale(pNodeAnim, timeKey);

		XMVECTOR xmvRotate = XMLoadFloat4(&keyFrame.xmf4RotationQuat);
		xmvRotate = XMQuaternionNormalize(xmvRotate);

		XMMATRIX xmmtxSrc = XMMatrixScaling(keyFrame.xmf3Scale.x, keyFrame.xmf3Scale.y, keyFrame.xmf3Scale.z) *
			XMMatrixRotationQuaternion(xmvRotate) * 
			XMMatrixTranslation(keyFrame.xmf3Position.x, keyFrame.xmf3Position.y, keyFrame.xmf3Position.z);
		
		XMFLOAT4X4 xmf4x4Src;
		XMStoreFloat4x4(&xmf4x4Src, xmmtxSrc);
		XMFLOAT4X4 xmf4x4Engine = ConvertMatrixToEngine(xmf4x4Src);
		XMMATRIX xmmtxEngine = XMLoadFloat4x4(&xmf4x4Engine);

		// Decompose SRT
		XMVECTOR xmvEngineScale;
		XMVECTOR xmvEngineRotate;
		XMVECTOR xmvEngineTranslate;
		if (!XMMatrixDecompose(&xmvEngineScale, &xmvEngineRotate, &xmvEngineTranslate, xmmtxEngine)) {
			// Fallback
			xmvEngineScale = XMVectorSet(1.f, 1.f, 1.f, 0.f);
			xmvEngineRotate = XMQuaternionIdentity();
			xmvEngineTranslate = XMVectorZero();
		}

		// Scale guard (incomplete)
		FixNegativeScaleAfterDecompose(xmvEngineScale, xmvEngineRotate);

		xmvEngineRotate = XMQuaternionNormalize(xmvEngineRotate);
		if (bHasPrevQuaternion) {
			float fDot = XMVectorGetX(XMVector4Dot(XMLoadFloat4(&xmf4PrevQuaternion), xmvEngineRotate));
			if (fDot < 0.f) {
				xmvEngineRotate = XMVectorNegate(xmvEngineRotate);
			}
		}

		XMStoreFloat3(&keyFrame.xmf3Scale, xmvEngineScale);
		XMStoreFloat4(&keyFrame.xmf4RotationQuat, xmvEngineRotate);
		XMStoreFloat3(&keyFrame.xmf3Position, xmvEngineTranslate);

		xmf4PrevQuaternion = keyFrame.xmf4RotationQuat;
		bHasPrevQuaternion = true;

		keyFrameDatas[timeKey / dTicksPerSecond] = keyFrame;
	}

	nodeAnim["nKeyFrames"] = keyFrameDatas.size();
	nodeAnim["KeyFrames"] = nlohmann::ordered_json::array();
	for (const auto& [fTimeKey, SRT] : keyFrameDatas) {
		nodeAnim["KeyFrames"].push_back(
			{ fTimeKey, 
				{
					SRT.xmf3Position.x, 
					SRT.xmf3Position.y, 
					SRT.xmf3Position.z
				}, 
				{
					SRT.xmf4RotationQuat.x,
					SRT.xmf4RotationQuat.y,
					SRT.xmf4RotationQuat.z,
					SRT.xmf4RotationQuat.w
				}, 
				{
					SRT.xmf3Scale.x,
					SRT.xmf3Scale.y,
					SRT.xmf3Scale.z
				} 
			}
		);
	}

	return nodeAnim;
}

XMFLOAT3 AssimpConverter::SamplePosition(const aiNodeAnim* pNodeAnim, double dTime) const
{
	// Check if no position key in node
	if (pNodeAnim->mNumPositionKeys == 0)
		return XMFLOAT3(0, 0, 0);

	float fScale = GetFinalScale();

	// Check if exact match is exist
	for (unsigned i = 0; i < pNodeAnim->mNumPositionKeys; ++i)
	{
		if (pNodeAnim->mPositionKeys[i].mTime == dTime)
		{
			XMFLOAT3 xmf3Pos = aiVector3DToXMVector(pNodeAnim->mPositionKeys[i].mValue);
			XMStoreFloat3(&xmf3Pos, XMVectorScale(XMLoadFloat3(&xmf3Pos), fScale));
			return xmf3Pos;
		}
	}

	// Handle out of range
	if (dTime <= pNodeAnim->mPositionKeys[0].mTime) {
		XMFLOAT3 xmf3Pos = aiVector3DToXMVector(pNodeAnim->mPositionKeys[0].mValue);
		XMStoreFloat3(&xmf3Pos, XMVectorScale(XMLoadFloat3(&xmf3Pos), fScale));
		return xmf3Pos;
	}

	if (dTime >= pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mTime) {
		XMFLOAT3 xmf3Pos = aiVector3DToXMVector(pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mValue);
		XMStoreFloat3(&xmf3Pos, XMVectorScale(XMLoadFloat3(&xmf3Pos), fScale));
		return xmf3Pos;
	}

	// find prev / next
	unsigned index = 0;
	for (; index < pNodeAnim->mNumPositionKeys - 1; ++index)
	{
		if (dTime < pNodeAnim->mPositionKeys[index + 1].mTime) {
			break;
		}
	}

	const aiVectorKey& key0 = pNodeAnim->mPositionKeys[index];
	const aiVectorKey& key1 = pNodeAnim->mPositionKeys[index + 1];

	double t = dTime;
	double t0 = key0.mTime;
	double t1 = key1.mTime;
	double alpha = (t - t0) / (t1 - t0);

	XMFLOAT3 xmf3Pos0 = aiVector3DToXMVector(key0.mValue);
	XMFLOAT3 xmf3Pos1 = aiVector3DToXMVector(key1.mValue);

	XMFLOAT3 xmf3Ret;
	XMStoreFloat3(&xmf3Ret, XMVectorLerp(XMLoadFloat3(&xmf3Pos0), XMLoadFloat3(&xmf3Pos1), static_cast<float>(alpha)));
	XMStoreFloat3(&xmf3Ret, XMVectorScale(XMLoadFloat3(&xmf3Ret), fScale));

	return xmf3Ret;
}

XMFLOAT4 AssimpConverter::SampleRotation(const aiNodeAnim* pNodeAnim, double dTime) const
{
	if (pNodeAnim->mNumRotationKeys == 0)
		return XMFLOAT4(0, 0, 0, 1); // identity quat

	for (unsigned i = 0; i < pNodeAnim->mNumRotationKeys; ++i)
	{
		if (pNodeAnim->mRotationKeys[i].mTime == dTime)
		{
			return aiQuaternionToXMVector(pNodeAnim->mRotationKeys[i].mValue);
		}
	}

	if (dTime <= pNodeAnim->mRotationKeys[0].mTime)
		return aiQuaternionToXMVector(pNodeAnim->mRotationKeys[0].mValue);

	if (dTime >= pNodeAnim->mRotationKeys[pNodeAnim->mNumRotationKeys - 1].mTime)
		return aiQuaternionToXMVector(pNodeAnim->mNumRotationKeys - 1 >= 0
			? pNodeAnim->mRotationKeys[pNodeAnim->mNumRotationKeys - 1].mValue
			: aiQuaternion(0, 0, 0, 1));

	unsigned index = 0;
	for (; index < pNodeAnim->mNumRotationKeys - 1; ++index)
	{
		if (dTime < pNodeAnim->mRotationKeys[index + 1].mTime)
			break;
	}

	const aiQuatKey& key0 = pNodeAnim->mRotationKeys[index];
	const aiQuatKey& key1 = pNodeAnim->mRotationKeys[index + 1];

	double t = dTime;
	double t0 = key0.mTime;
	double t1 = key1.mTime;
	double alpha = (t - t0) / (t1 - t0);

	XMFLOAT4 xmf4Rot0 = aiQuaternionToXMVector(key0.mValue);
	XMFLOAT4 xmf4Rot1 = aiQuaternionToXMVector(key1.mValue);

	XMFLOAT4 xmf4Ret;
	XMStoreFloat4(&xmf4Ret, XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&xmf4Rot0), XMLoadFloat4(&xmf4Rot1), static_cast<float>(alpha))));
	return xmf4Ret;
}

XMFLOAT3 AssimpConverter::SampleScale(const aiNodeAnim* pNodeAnim, double dTime) const
{
	if (pNodeAnim->mNumScalingKeys == 0)
		return XMFLOAT3(1, 1, 1);

	for (unsigned i = 0; i < pNodeAnim->mNumScalingKeys; ++i)
	{
		if (pNodeAnim->mScalingKeys[i].mTime == dTime)
		{
			return aiVector3DToXMVector(pNodeAnim->mScalingKeys[i].mValue);
		}
	}

	if (dTime <= pNodeAnim->mScalingKeys[0].mTime)
		return aiVector3DToXMVector(pNodeAnim->mScalingKeys[0].mValue);

	if (dTime >= pNodeAnim->mScalingKeys[pNodeAnim->mNumScalingKeys - 1].mTime)
		return aiVector3DToXMVector(pNodeAnim->mScalingKeys[pNodeAnim->mNumScalingKeys - 1].mValue);

	unsigned index = 0;
	for (; index < pNodeAnim->mNumScalingKeys - 1; ++index)
	{
		if (dTime < pNodeAnim->mScalingKeys[index + 1].mTime)
			break;
	}

	const aiVectorKey& key0 = pNodeAnim->mScalingKeys[index];
	const aiVectorKey& key1 = pNodeAnim->mScalingKeys[index + 1];

	double t = dTime;
	double t0 = key0.mTime;
	double t1 = key1.mTime;
	double alpha = (t - t0) / (t1 - t0);

	XMFLOAT3 xmf3Scale0 = aiVector3DToXMVector(key0.mValue);
	XMFLOAT3 xmf3Scale1 = aiVector3DToXMVector(key1.mValue);

	XMFLOAT3 xmf3Ret;
	XMStoreFloat3(&xmf3Ret, XMVectorLerp(XMLoadFloat3(&xmf3Scale0), XMLoadFloat3(&xmf3Scale1), static_cast<float>(alpha)));
	return xmf3Ret;
}

bool AssimpConverter::IsDDS(const aiTexture* tex)
{
	if (tex->mHeight != 0)
		return false;

	if (tex->mWidth < 4)
		return false;

	const char* data = reinterpret_cast<const char*>(tex->pcData);
	return memcmp(data, "DDS ", 4) == 0;
}

XMFLOAT4X4 AssimpConverter::ConvertMatrixToEngine(const XMFLOAT4X4& xmf4x4Matrix) const
{
	XMFLOAT4X4 xmf4x4Return;

	XMMATRIX xmmtxSceneToEngine = XMLoadFloat4x4(&m_xmf4x4SourceToEngine);

	XMMATRIX xmmtxInverseSceneToEngine = XMMatrixInverse(nullptr, xmmtxSceneToEngine);
	XMMATRIX xmmtxTransform = XMLoadFloat4x4(&xmf4x4Matrix);
	XMMATRIX xmmtxFinalTransform = xmmtxInverseSceneToEngine * xmmtxTransform * xmmtxSceneToEngine;

	XMStoreFloat4x4(&xmf4x4Return, xmmtxFinalTransform);

	return xmf4x4Return;
}

void AssimpConverter::FixNegativeScaleAfterDecompose(XMVECTOR& xmvScale, XMVECTOR& xmvRotate) const
{
	XMMATRIX xmmtxRotate = XMMatrixRotationQuaternion(xmvRotate);
	float fDeterminant = XMVectorGetX(XMVector3Dot(XMVector3Cross(xmmtxRotate.r[0], xmmtxRotate.r[1]), xmmtxRotate.r[2]));
	if (fDeterminant >= 0.f) {
		return;
	}

	// Select axis to negate
	XMFLOAT3 xmf3Scale;
	XMStoreFloat3(&xmf3Scale, xmvScale);
	float ax = fabsf(xmf3Scale.x);
	float ay = fabsf(xmf3Scale.y);
	float az = fabsf(xmf3Scale.z);

	int nAxis = 0;
	float fBest = ax;
	if (ay < fBest) {
		fBest = ay;
		nAxis = 1;
	}
	if (az < fBest) {
		nAxis = 2;
	}

	// Nagate Axis of xmvScale
	if (nAxis == 0) xmf3Scale.x = -xmf3Scale.x;
	if (nAxis == 1) xmf3Scale.y = -xmf3Scale.y;
	if (nAxis == 2) xmf3Scale.z = -xmf3Scale.z;
	xmvScale = XMLoadFloat3(&xmf3Scale);

	if (nAxis == 0) xmmtxRotate.r[0] = XMVectorNegate(xmmtxRotate.r[0]);
	if (nAxis == 1) xmmtxRotate.r[1] = XMVectorNegate(xmmtxRotate.r[1]);
	if (nAxis == 2) xmmtxRotate.r[2] = XMVectorNegate(xmmtxRotate.r[2]);

	xmvRotate = XMQuaternionNormalize(XMQuaternionRotationMatrix(xmmtxRotate));
}

float AssimpConverter::GetFinalScale() const
{
	return m_dUnitScaleCM * m_fScale;
}

void AssimpConverter::FlipNormalMapY(DirectX::ScratchImage& scratchImage) const
{
	const TexMetadata& metaData = scratchImage.GetMetadata();

	if (metaData.dimension != TEX_DIMENSION_TEXTURE2D) {
		return;
	}

	if (metaData.format != DXGI_FORMAT_R8G8B8A8_UNORM &&
		metaData.format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
		return;
	}

	for (size_t i = 0; i < scratchImage.GetImageCount(); ++i) {
		const Image* img = scratchImage.GetImage(i, 0, 0);
		uint8_t* pPixels = img->pixels;

		for (size_t y = 0; y < img->height; ++y) {
			uint8_t* pRow = pPixels + y * img->rowPitch;
			for (size_t x = 0; x < img->width; ++x) {
				uint8_t* pRGB = pRow + x * 4; // [0] : R, [1] : G, [2] : B
				pRGB[1] = 255 - pRGB[1];
			}
		}

	}
}
