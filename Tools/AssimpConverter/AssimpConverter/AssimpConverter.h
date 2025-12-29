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
	std::array<UINT, 4>		uiBlendIndices = { 0,0,0,0 };
	std::array<float, 4>	fBlendWeights = { 0,0,0,0 };
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

struct Bone {	// == Frame
	std::string strFrameName;
	XMFLOAT4X4 Transform;

	int nParentIdx = -1;
	std::vector<int> nChildrenIdx;
};

class AssimpConverter {
public:
	AssimpConverter();
	~AssimpConverter();

	void LoadFromFiles(const std::string& strPath, float fScaleFactor = 1.f);

public:
	void Serialize(const std::string& strPath, const std::string& strName) const;


public:
	void ShowFrameData() const;

private:
	void ProcessFrameData(const aiNode* pNode, UINT nTabs = 0) const;
	void ProcessKeyframeData(const aiNodeAnim* pNode, UINT nTabs = 0) const;

	void CountBones(const aiNode* pNode, int& nNumBones);
	void StoreBoneData(const aiNode* pNode);
	void StoreBoneHeirachy(const aiNode* pNode);
	
	nlohmann::ordered_json StoreNodeToJson(const aiNode* pNode) const;
	nlohmann::ordered_json StoreMeshToJson(const aiMesh* pMesh) const;
	nlohmann::ordered_json StoreMaterialToJson(const aiMaterial* pMaterial) const;


private:
	std::shared_ptr<Assimp::Importer> m_pImporter = nullptr;
	const aiScene* m_pScene = nullptr;
	aiNode* m_pRootNode = nullptr;
	UINT m_nNodes = 0;

	std::vector<Bone> m_Bones;
	std::unordered_map<std::string, UINT> m_BoneIndexMap;

	float m_fScale = 1.f;

	std::string m_strFilePath;




};

