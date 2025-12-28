#pragma once

struct Mesh {
	std::vector<XMFLOAT3> xmf3Positions;
	std::vector<XMFLOAT3> xmf3Normals;
	std::vector<XMFLOAT3> xmf3Tangents;
	std::vector<XMFLOAT2> xmf3TexCoords;

	std::vector<UINT> uiIndices;

	BoundingBox xmAABB;
};

struct SkinData {
	std::vector<XMINT4>		xmi4BlendIndices;
	std::vector<XMFLOAT4>	xmf4BlendWeights;
};

enum MATERIAL_TYPE {
	MATERIAL_TYPE_ALBEDO = 0x01,
	MATERIAL_TYPE_SPECULAR = 0x02,
	MATERIAL_TYPE_METALLIC = 0x04,
	MATERIAL_TYPE_NORMAL = 0x08,
	MATERIAL_TYPE_EMISSION = 0x10,
	MATERIAL_TYPE_DETAIL_ALBEDO = 0x20,
	MATERIAL_TYPE_DETAIL_NORMAL = 0x40,
};

struct Material {
	std::string		strMaterialName;

	XMFLOAT4		xmf4AlbedoColor;
	XMFLOAT4		xmf4SpecularColor;
	XMFLOAT4		xmf4AmbientColor;
	XMFLOAT4		xmf4EmissiveColor;

	float			fGlossiness = 0.0f;
	float			fSmoothness = 0.0f;
	float			fSpecularHighlight = 0.0f;
	float			fMetallic = 0.0f;
	float			fGlossyReflection = 0.0f;

	std::string		strAlbedoMapName;
	std::string		strSpecularMapName;
	std::string		strMetallicMapName;
	std::string		strNormalMapName;
	std::string		strEmissionMapName;
	std::string		strDetailAlbedoMapName;
	std::string		strDetailNormalMapName;

	UINT nType;

};

struct Animation {

};

struct Bone {
	std::string strFrameName;
	XMFLOAT4X4 xmf4x4Offset;

	int nParentIdx = -1;
	std::vector<int> nChildrenIdx;
};

class AssimpConverter {
public:
	AssimpConverter();
	~AssimpConverter();

	void LoadFromFiles(const std::string& strPath);

	void ShowFrameData() const;
	void ShowMeshData() const;
	void ShowBoneData() const;

private:
	void ProcessFrameData(const aiNode* pNode, UINT nTabs = 0) const;
	void ProcessMeshData(const aiNode* pNode, UINT nTabs = 0) const;
	void ProcessMaterialData(const aiMesh* pMesh, UINT nTabs = 0) const;
	void ProcessBoneData(const aiBone* pBone, UINT nTabs = 0) const;
	void ProcessKeyframeData(const aiNodeAnim* pNode, UINT nTabs = 0) const;

	void CountBones(const aiNode* pNode, int& nNumBones);
	void StoreBoneData(const aiNode* pNode);
	void StoreBoneHeirachy(const aiNode* pNode);

private:
	std::shared_ptr<Assimp::Importer> m_pImporter = nullptr;
	const aiScene* m_pScene = nullptr;
	aiNode* m_pRootNode = nullptr;
	UINT m_nNodes = 0;

	std::vector<Bone> m_Bones;
	std::unordered_map<aiString, UINT> m_BoneIndexMap;

	nlohmann::json m_json{};

};

