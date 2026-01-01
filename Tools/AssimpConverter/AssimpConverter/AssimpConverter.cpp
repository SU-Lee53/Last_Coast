#include "stdafx.h"
#include "AssimpConverter.h"

AssimpConverter::AssimpConverter()
{
	m_pImporter = std::make_shared<Assimp::Importer>();
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
		aiProcess_LimitBoneWeights	// limits influencing bone count to for per vertex
	);

	if (!m_pScene) {
		__debugbreak();
	}

	m_pRootNode = m_pScene->mRootNode;
	
	GatherBoneIndex();
	BuildBoneHierarchy(m_pRootNode, -1);
}

void AssimpConverter::GatherBoneIndex()
{
	for (int m = 0; m < m_pScene->mNumMeshes; ++m)
	{
		const aiMesh* pMesh = m_pScene->mMeshes[m];

		for (int b = 0; b < pMesh->mNumBones; ++b)
		{
			const aiBone* pBone = pMesh->mBones[b];
			std::string strName = pBone->mName.C_Str();

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
	std::string nodeName = node->mName.C_Str();
	int currentBoneIndex = -1;

	auto it = m_BoneIndexMap.find(nodeName);
	if (it != m_BoneIndexMap.end())
	{
		currentBoneIndex = it->second;
		m_Bones[currentBoneIndex].nParentIndex = parentBoneIndex;
		m_Bones[currentBoneIndex].xmf4x4Transform = aiMatrixToXMMatrix(node->mTransformation);
	}

	for (int i = 0; i < node->mNumChildren; ++i)
	{
		BuildBoneHierarchy(
			node->mChildren[i],
			currentBoneIndex == -1 ? parentBoneIndex : currentBoneIndex
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

	strSave = std::format("{}\\Models\\{}.json", m_strSavePath, strName);
	std::ofstream out(strSave);

	DisplayText("Serializing...\n");

	nlohmann::ordered_json hierarchyJson;
	hierarchyJson["Hierarchy"] = StoreNodeToJson(m_pRootNode);

	// Bone data (For animation retargeting)
	hierarchyJson["nBones"] = m_Bones.size();
	hierarchyJson["Bones"] = nlohmann::ordered_json::array();
	for (const auto& boneData : m_Bones) {
		nlohmann::ordered_json bone;
		bone["Name"] = boneData.strName;
		bone["Index"] = boneData.nIndex;
		bone["ParentIndex"] = boneData.nParentIndex;
		XMFLOAT4X4 m = boneData.xmf4x4Transform;
		bone["localBind"] = {
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44,
		};

		m = boneData.xmf4x4Offset;
		bone["inverseBind"] = {
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44,
		};

		hierarchyJson["Bones"].push_back(bone);
	}

	out << hierarchyJson.dump(2);


	DisplayText("Successfully serialized at %s\r\n", m_strSavePath.c_str());

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
	mesh["Tangents"] = nlohmann::ordered_json::array();
	if (pMesh->mTangents) {
		for (int i = 0; i < pMesh->mNumVertices; ++i) {
			aiVector3D v3Tangents = pMesh->mTangents[i];
			mesh["Tangents"].push_back(v3Tangents.x);
			mesh["Tangents"].push_back(v3Tangents.y);
			mesh["Tangents"].push_back(v3Tangents.z);
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
			mesh["BiTangents"].push_back(v3BiTangents.x);
			mesh["BiTangents"].push_back(v3BiTangents.y);
			mesh["BiTangents"].push_back(v3BiTangents.z);
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
	else {
		mesh["Skinned?"] = false;
		mesh["BlendIndices"] = nullptr;
		mesh["BlendWeights"] = nullptr;
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
				if (pTexture) {
					ExportEmbeddedTexture(pTexture, eType);

					std::string strTextureName = pTexture->mFilename.C_Str();
					strTextureName = std::filesystem::path{ strTextureName }.stem().string();
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
				else {
					// External texture
					ExportExternalTexture(aistrTexturePath, eType);
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

void AssimpConverter::SerializeAnimation(const std::string& strPath, const std::string& strName)
{
	if (m_pScene->mNumAnimations == 0) {
		DisplayText("File doesn't have keyframe animation");
	}

	namespace fs = std::filesystem;
	m_strSavePath = strPath + '\\' + strName;

	// 1. Make save path (if not exists)
	fs::path saveDirectoryPath{ m_strSavePath };
	if (!fs::exists(saveDirectoryPath)) {
		fs::create_directories(m_strSavePath);
	}

	// 2. Defines Savefile name
	std::string strSave = std::format("{}/{}.json", m_strSavePath, strName);
	std::ofstream out(strSave);

	DisplayText("Serializing...\n");

	nlohmann::ordered_json animJson = nlohmann::ordered_json::array();
	for (int i = 0; i < m_pScene->mNumAnimations; ++i) {
		animJson.push_back(StoreAnimationToJson(m_pScene->mAnimations[i]));
	}

	out << animJson.dump(2);


	DisplayText("Successfully serialized at %s\r\n", m_strSavePath.c_str());

}

nlohmann::ordered_json AssimpConverter::StoreAnimationToJson(const aiAnimation* pAnimation) const
{
	// These two below are NOT keyframe animation data
	//	pAnimation->mMeshChannels; -> Tweening
	//	pAnimation->mMorphMeshChannels; -> Morphing
	nlohmann::ordered_json anim;
	anim["Name"] = pAnimation->mName.C_Str();
	anim["Duration"] = pAnimation->mDuration;
	anim["TicksPerSecond"] = pAnimation->mTicksPerSecond;

	anim["nChannels"] = pAnimation->mNumChannels;
	anim["Channels"] = nlohmann::ordered_json::array();
	for (int i = 0; i < pAnimation->mNumChannels; ++i) {
		anim["Channels"].push_back(StoreNodeAnimToJson(pAnimation->mChannels[i]));
	}

	return anim;
}

nlohmann::ordered_json AssimpConverter::StoreNodeAnimToJson(const aiNodeAnim* pNodeAnim) const
{
	nlohmann::ordered_json nodeAnim;
	nodeAnim["Name"] = pNodeAnim->mNodeName.C_Str();	// name of bone

	//	for (int i = 0; i < pNodeAnim->mNumPositionKeys; ++i) {
	//		aiVectorKey keyFrame = pNodeAnim->mPositionKeys[i];
	//		keyFrameDatas[keyFrame.mTime].xmf3Position = aiVector3DToXMVector(keyFrame.mValue);
	//	}
	//	
	//	for (int i = 0; i < pNodeAnim->mNumRotationKeys; ++i) {
	//		aiQuatKey keyFrame = pNodeAnim->mRotationKeys[i];
	//		auto it = keyFrameDatas.find(keyFrame.mTime);
	//		if (it != keyFrameDatas.end()) {
	//			keyFrameDatas[keyFrame.mTime].xmf4RotationQuat = aiQuaternionToXMVector(keyFrame.mValue);
	//		}
	//		else {
	//			// Interpolate
	//			KeyFrame newKeyFrame;
	//	
	//			keyFrameDatas.insert({ keyFrame.mTime, {} });
	//			auto it = keyFrameDatas.find(keyFrame.mTime);
	//			if (it == keyFrameDatas.begin()) {
	//				auto nextKeyFrame = *std::next(it);
	//				newKeyFrame.xmf3Position = nextKeyFrame.second.xmf3Position;
	//				newKeyFrame.xmf4RotationQuat = aiQuaternionToXMVector(keyFrame.mValue);
	//				newKeyFrame.xmf3Scale = nextKeyFrame.second.xmf3Scale;
	//			}
	//			else if (std::next(it) == keyFrameDatas.end()) {
	//				auto prevKeyFrame = *std::prev(it);
	//				newKeyFrame.xmf3Position = prevKeyFrame.second.xmf3Position;
	//				newKeyFrame.xmf4RotationQuat = aiQuaternionToXMVector(keyFrame.mValue);
	//				newKeyFrame.xmf3Scale = prevKeyFrame.second.xmf3Scale;
	//			}
	//			else {
	//				auto prevKeyFrame = *std::prev(it);
	//				auto nextKeyFrame = *std::next(it);
	//	
	//				double t = keyFrame.mTime;
	//				double t0 = prevKeyFrame.first;
	//				double t1 = nextKeyFrame.first;
	//	
	//				double alpha = (t - t0) / (t1 - t0);
	//	
	//				XMStoreFloat3(&newKeyFrame.xmf3Position, XMVectorLerp(XMLoadFloat3(&prevKeyFrame.second.xmf3Position), XMLoadFloat3(&nextKeyFrame.second.xmf3Position), alpha));
	//				XMStoreFloat3(&newKeyFrame.xmf3Scale, XMVectorLerp(XMLoadFloat3(&prevKeyFrame.second.xmf3Scale), XMLoadFloat3(&nextKeyFrame.second.xmf3Scale), alpha));
	//				newKeyFrame.xmf4RotationQuat = aiQuaternionToXMVector(keyFrame.mValue);
	//			}
	//	
	//			it->second = newKeyFrame;
	//		}
	//	}
	//	
	//	for (int i = 0; i < pNodeAnim->mNumScalingKeys; ++i) {
	//		aiVectorKey keyFrame = pNodeAnim->mScalingKeys[i];
	//		auto it = keyFrameDatas.find(keyFrame.mTime);
	//		if (it != keyFrameDatas.end()) {
	//			keyFrameDatas[keyFrame.mTime].xmf3Scale = aiVector3DToXMVector(keyFrame.mValue);
	//		}
	//		else {
	//			// Interpolate
	//			KeyFrame newKeyFrame;
	//	
	//			keyFrameDatas.insert({ keyFrame.mTime, {} });
	//			auto it = keyFrameDatas.find(keyFrame.mTime);
	//			if (it == keyFrameDatas.begin()) {
	//				auto nextKeyFrame = *std::next(it);
	//				newKeyFrame.xmf3Position = nextKeyFrame.second.xmf3Position;
	//				newKeyFrame.xmf4RotationQuat = nextKeyFrame.second.xmf4RotationQuat;
	//				newKeyFrame.xmf3Scale = aiVector3DToXMVector(keyFrame.mValue);
	//			}
	//			else if (std::next(it) == keyFrameDatas.end()) {
	//				auto prevKeyFrame = *std::prev(it);
	//				newKeyFrame.xmf3Position = prevKeyFrame.second.xmf3Position;
	//				newKeyFrame.xmf4RotationQuat = prevKeyFrame.second.xmf4RotationQuat;
	//				newKeyFrame.xmf3Scale = aiVector3DToXMVector(keyFrame.mValue);
	//			}
	//			else {
	//				auto prevKeyFrame = *std::prev(it);
	//				auto nextKeyFrame = *std::next(it);
	//	
	//				double t = keyFrame.mTime;
	//				double t0 = prevKeyFrame.first;
	//				double t1 = nextKeyFrame.first;
	//	
	//				double alpha = (t - t0) / (t1 - t0);
	//	
	//				XMStoreFloat3(&newKeyFrame.xmf3Position, XMVectorLerp(XMLoadFloat3(&prevKeyFrame.second.xmf3Position), XMLoadFloat3(&nextKeyFrame.second.xmf3Position), alpha));
	//				XMStoreFloat4(&newKeyFrame.xmf4RotationQuat, XMQuaternionSlerp(XMLoadFloat4(&prevKeyFrame.second.xmf4RotationQuat), XMLoadFloat4(&nextKeyFrame.second.xmf4RotationQuat), alpha));
	//				newKeyFrame.xmf3Scale = aiVector3DToXMVector(keyFrame.mValue);
	//			}
	//	
	//			it->second = newKeyFrame;
	//		}
	//	}

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

	for (double key : keys)
	{
		KeyFrame keyFrame;
		keyFrame.xmf3Position = SamplePosition(pNodeAnim, key);
		keyFrame.xmf4RotationQuat = SampleRotation(pNodeAnim, key);
		keyFrame.xmf3Scale = SampleScale(pNodeAnim, key);

		keyFrameDatas[key] = keyFrame;
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

	//nodeAnim["nPositionKeys"] = pNodeAnim->mNumPositionKeys;
	//nodeAnim["PositionKeys"] = nlohmann::ordered_json::array();
	//for (int i = 0; i < pNodeAnim->mNumPositionKeys; ++i) {
	//	aiVectorKey keyFrame = pNodeAnim->mPositionKeys[i];
	//	nodeAnim["PositionKeys"].push_back({ keyFrame.mTime, { keyFrame.mValue.x, keyFrame.mValue.y, keyFrame.mValue.z } });
	//}
	//
	//nodeAnim["nRotationKeys"] = pNodeAnim->mNumRotationKeys;
	//nodeAnim["RotationKeys"] = nlohmann::ordered_json::array();
	//for (int i = 0; i < pNodeAnim->mNumRotationKeys; ++i) {
	//	aiQuatKey keyFrame = pNodeAnim->mRotationKeys[i];
	//	nodeAnim["RotationKeys"].push_back({ keyFrame.mTime, { keyFrame.mValue.x, keyFrame.mValue.y, keyFrame.mValue.z, keyFrame.mValue.w } });
	//}
	//
	//nodeAnim["nScalingKeys"] = pNodeAnim->mNumScalingKeys;
	//nodeAnim["ScalingKeys"] = nlohmann::ordered_json::array();
	//for (int i = 0; i < pNodeAnim->mNumRotationKeys; ++i) {
	//	aiVectorKey keyFrame = pNodeAnim->mScalingKeys[i];
	//	nodeAnim["ScalingKeys"].push_back({ keyFrame.mTime, { keyFrame.mValue.x, keyFrame.mValue.y, keyFrame.mValue.z } });
	//}

	return nodeAnim;
}

XMFLOAT3 AssimpConverter::SamplePosition(const aiNodeAnim* pNodeAnim, double dTime) const
{
	// Check if no position key in node
	if (pNodeAnim->mNumPositionKeys == 0)
		return XMFLOAT3(0, 0, 0);

	// Check if exact match is exist
	for (unsigned i = 0; i < pNodeAnim->mNumPositionKeys; ++i)
	{
		if (pNodeAnim->mPositionKeys[i].mTime == dTime)
		{
			return aiVector3DToXMVector(pNodeAnim->mPositionKeys[i].mValue);
		}
	}

	// Handle out of range
	if (dTime <= pNodeAnim->mPositionKeys[0].mTime)
		return aiVector3DToXMVector(pNodeAnim->mPositionKeys[0].mValue);

	if (dTime >= pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mTime)
		return aiVector3DToXMVector(pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mValue);

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
