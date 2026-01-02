#pragma once
#include "Mesh.h"
#include "Material.h"
#include "MeshRenderer.h"

class GameObject;

enum OBJECT_RENDER_TYPE : UINT {
	OBJECT_RENDER_FORWARD = 0,
	OBJECT_RENDER_DIFFERED,

	OBJECT_RENDER_TYPE_COUNT
};

class IMeshRenderer abstract : public std::enable_shared_from_this<IMeshRenderer> {
public:
	using mesh_type = nullptr_t;
	using material_type = nullptr_t;
	friend struct std::hash<IMeshRenderer>;

public:
	IMeshRenderer();
	IMeshRenderer(const IMeshRenderer& other);
	IMeshRenderer(IMeshRenderer&& other);

	IMeshRenderer& operator=(const IMeshRenderer& other);
	IMeshRenderer& operator=(IMeshRenderer&& other);

	virtual ~IMeshRenderer() {}
	virtual void Initialize() = 0;
	virtual void Update(std::shared_ptr<GameObject> pOwner) = 0;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase, const Matrix& mtxWorld = Matrix::Identity) const = 0;

	bool operator==(const IMeshRenderer& rhs) const;

	const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_pMeshes; }
	const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_pMaterials; };

	uint64_t GetID() const { return m_ui64RendererID; }
	UINT GetRenderType() const { return m_eRenderType; }


	void SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType);

protected:
	std::vector<std::shared_ptr<Mesh>> m_pMeshes;
	std::vector<std::shared_ptr<Material>> m_pMaterials;

	uint64_t m_ui64RendererID = 0;
	UINT m_eRenderType = std::numeric_limits<UINT>::max();

protected:
	static uint64_t g_ui64RendererIDBase;

};

template<typename meshType, UINT eRenderType = OBJECT_RENDER_FORWARD>
class MeshRenderer : public IMeshRenderer {
public:
	using mesh_type = meshType;
	using material_type = typename std::conditional_t<std::same_as<meshType, StandardMesh>, StandardMaterial, SkinnedMaterial>;
	friend struct std::hash<IMeshRenderer>;

public:
	MeshRenderer() = default;
	MeshRenderer(const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo);
	virtual ~MeshRenderer() {}

public:
	virtual void Initialize() override;
	virtual void Update(std::shared_ptr<GameObject> pOwner)override;
	virtual void Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
		DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase, const Matrix& mtxWorld = Matrix::Identity) const override;
};

template<>
struct std::hash<IMeshRenderer> {
	size_t operator()(const IMeshRenderer& meshRenderer) const {
		return (std::hash<decltype(meshRenderer.m_pMeshes)::value_type>{}(meshRenderer.m_pMeshes[0]) ^
			std::hash<decltype(meshRenderer.m_pMaterials)::value_type>{}(meshRenderer.m_pMaterials[0]));
	}
};

template<typename meshType, UINT eRenderType>
inline MeshRenderer<meshType, eRenderType>::MeshRenderer(const std::vector<MESHLOADINFO>& meshLoadInfos, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{
	m_pMeshes.reserve(meshLoadInfos.size());

	for (const auto& meshLoadInfo : meshLoadInfos) {
		std::shared_ptr<meshType> pMesh = std::make_shared<meshType>(meshLoadInfo);
		m_pMeshes.push_back(pMesh);
	}

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<Material> pMaterial;
		if constexpr (std::same_as<meshType, StandardMesh>) {
			pMaterial = std::make_shared<StandardMaterial>(materialInfo);
		}
		else {
			pMaterial = std::make_shared<SkinnedMaterial>(materialInfo);
		}
		m_pMaterials.push_back(pMaterial);
	}
}

template<typename meshType, UINT eRenderType>
inline void MeshRenderer<meshType, eRenderType>::Initialize()
{
	m_eRenderType = eRenderType;
}

template<typename meshType, UINT eRenderType>
inline void MeshRenderer<meshType, eRenderType>::Update(std::shared_ptr<GameObject> pOwner)
{
	MeshRenderParameters meshParam{
		.mtxWorld = pOwner->GetWorldMatrix().Transpose()
	};
#ifdef WITH_FRUSTUM_CULLING

#else
	for (int i = 0; i < m_pMeshes.size(); ++i) {
		RENDER->Add(shared_from_this(), meshParam);
	}
#endif
}

template<typename meshType, UINT eRenderType>
inline void MeshRenderer<meshType, eRenderType>::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, 
	DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase, const Matrix& mtxWorld) const
{
	for (int i = 0; i < m_pMeshes.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), refnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		if constexpr (std::same_as <meshType, SkinnedMesh>) {
			Matrix mtxWorldTransposed = mtxWorld.Transpose();
			ConstantBuffer worldCBuffer = RESOURCE->AllocCBuffer<CB_WORLD_TRANSFORM_DATA>();
			worldCBuffer.WriteData(&mtxWorldTransposed);

			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, worldCBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			descHandle.cpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);

			pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_OBJ_MATERIAL_DATA, descHandle.gpuHandle);
			descHandle.gpuHandle.Offset(2, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}
		else {
			// 4
			pd3dDevice->CopyDescriptorsSimple(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_OBJ_MATERIAL_DATA, descHandle.gpuHandle);
			descHandle.cpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);
			descHandle.gpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		}


		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMeshes[i]->Render(pd3dCommandList, 0, nInstanceCount);
	}
	refnInstanceBase += nInstanceCount;
}
