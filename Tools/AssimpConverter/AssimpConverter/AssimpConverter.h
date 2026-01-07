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

struct Bone {	// == Frame
	std::string strName;
	int nIndex;
	int nParentIndex;
	XMFLOAT4X4 xmf4x4Transform;	// aiNode::mTransformation
	XMFLOAT4X4 xmf4x4Offset;	// Inverse bind pose

	int nChildren = 0;
	std::vector<int> nChilerenIndex;
	int nDepth = 0;

};

struct KeyFrame {
	XMFLOAT3 xmf3Position{ 0.f, 0.f, 0.f };
	XMFLOAT4 xmf4RotationQuat{ 0.f, 0.f, 0.f, 1.f };
	XMFLOAT3 xmf3Scale{ 1.f, 1.f, 1.f };
};

struct SceneAxis {
	int nUpAxis = -1;
	int nUpSign = 1;
	int nFrontAxis = -1;
	int nFrontSign = 1;
	int nCoordAxis = -1;	// Right
	int nCoordSign = 1;		// Right
	bool bHasMetadata = false;
};

struct Basis3 {
	XMFLOAT3 xmf3Right;
	XMFLOAT3 xmf3Up;
	XMFLOAT3 xmf3Forward;
};

class AssimpConverter {
public:
	AssimpConverter();

	bool IsOpened() { return m_pScene; }
	void LoadFromFiles(const std::string& strPath, float fScaleFactor = 1.f);

	void SerializeModel(const std::string& strPath, const std::string& strName);
	void SerializeAnimation(const std::string& strPath, const std::string& strName);

private:
	SceneAxis ReadSceneAxisMetaData(const aiScene* pScene);
	XMFLOAT3 AxisToVector(int nAxisIndex, int nSign);
	Basis3 MakeSourceBasis(const SceneAxis& metaData);
	bool IsRightHanded(const Basis3& basis);
	void ApplyReflectionRH(Basis3& basis, bool& outbSceneWasRH);
	XMFLOAT4X4 MakeBasisMatrix(const Basis3& basis);
	XMFLOAT4X4 BuildSourceToEngineMatrix(const SceneAxis& metaData, bool& outbWasRH);


	void GatherBoneIndex(); 
	void BuildBoneHierarchy(aiNode* node, int parentBoneIndex);
	
	nlohmann::ordered_json StoreNodeToJson(const aiNode* pNode) const;
	nlohmann::ordered_json StoreMeshToJson(const aiMesh* pMesh) const;
	nlohmann::ordered_json StoreMaterialToJson(const aiMaterial* pMaterial) const;
	nlohmann::ordered_json StoreAnimationToJson(const aiAnimation* pAnimation) const;
	nlohmann::ordered_json StoreNodeAnimToJson(const aiNodeAnim* pNodeAnim, double dTicksPerSecond) const;

	void ExportEmbeddedTexture(const aiTexture* pTexture, aiTextureType eTextureType) const;
	void ExportExternalTexture(const aiString& aistrTexturePath, aiTextureType eTextureType) const;
	void FlipNormalMapY(DirectX::ScratchImage& img) const;

	XMFLOAT3 SamplePosition(const aiNodeAnim* pNodeAnim, double dTime) const;
	XMFLOAT4 SampleRotation(const aiNodeAnim* pNodeAnim, double dTime) const;
	XMFLOAT3 SampleScale(const aiNodeAnim* pNodeAnim, double dTime) const;

	XMFLOAT4X4 ConvertMatrixToEngine(const XMFLOAT4X4& xmf4x4Matrix) const;
	void FixNegativeScaleAfterDecompose(XMVECTOR& xmvScale, XMVECTOR& xmvRotate) const;

private:
	std::shared_ptr<Assimp::Importer> m_pImporter = nullptr;
	const aiScene* m_pScene = nullptr;
	aiNode* m_pRootNode = nullptr;
	UINT m_nNodes = 0;

	std::vector<Bone> m_Bones;
	std::vector<Bone> m_DFSBones;
	std::unordered_map<int, int> m_BoneIndexRemappedDFS;	// old(not DFS order) - new(DFS order)
	std::unordered_map<std::string, UINT> m_BoneIndexMap;

	float m_fScale = 1.f;

	std::string m_strFilePath;
	std::string m_strSavePath;

	XMFLOAT4X4 m_xmf4x4SourceToEngine;
	bool m_bSourceWasRH = false;

private:
	static bool IsDDS(const aiTexture* tex);

};

