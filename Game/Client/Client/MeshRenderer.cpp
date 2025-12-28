#include "pch.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"

MeshRenderer::MeshRenderer()
{
}

MeshRenderer::MeshRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials)
{
}

MeshRenderer::MeshRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{
}

MeshRenderer::MeshRenderer(const MeshRenderer& other)
{
	*this = other;
}

MeshRenderer::MeshRenderer(MeshRenderer&& other)
{
	*this = std::move(other);
}

MeshRenderer& MeshRenderer::operator=(const MeshRenderer& other)
{
	if (this == &other) {
		return *this;
	}

	m_pMesh = other.m_pMesh;
	m_pMaterials = other.m_pMaterials;

	return *this;
}

// 문제 있을 "수도" 있음 나중에 확인 필요
MeshRenderer& MeshRenderer::operator=(MeshRenderer&& other)
{
	if (this == &other) {
		return *this;
	}

	m_pMesh = std::move(other.m_pMesh);
	m_pMaterials = std::move(other.m_pMaterials);

	return *this;
}

bool MeshRenderer::operator==(const MeshRenderer& rhs) const
{
	return m_pMesh == rhs.m_pMesh && m_pMaterials == rhs.m_pMaterials;
}

void MeshRenderer::SetTexture(std::shared_ptr<Texture> pTexture, UINT nMaterialIndex, TEXTURE_TYPE eTextureType)
{
	assert(pTexture);
	m_pMaterials[nMaterialIndex]->SetTexture(pTexture, eTextureType);
}

void MeshRenderer::Initialize()
{
}

void MeshRenderer::Update(std::shared_ptr<GameObject> pOwner)
{
	MeshRenderParameters meshParam{
		.mtxWorld = pOwner->GetWorldMatrix().Transpose()
	};
#ifdef WITH_FRUSTUM_CULLING
	
#else
	if (m_bAddToRenderManager && (m_pMesh && m_pMaterials.size() != 0)) {
		RENDER->Add<MeshRenderer>(shared_from_this(), meshParam);
	}
#endif
}


//////////////////////////////////////////////////////////////////////////////////
// DiffusedRenderer

DiffusedRenderer::DiffusedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials)
{
	m_eObjectRenderType = OBJECT_RENDER_FORWARD;
	m_pMesh = pMesh;
	m_pMaterials = pMaterials;
}

DiffusedRenderer::DiffusedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{ 
	m_eObjectRenderType = OBJECT_RENDER_FORWARD;
	m_pMesh = std::make_shared<DiffusedMesh>(meshLoadInfo);

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<DiffusedMaterial> pMaterial = std::make_shared<DiffusedMaterial>(materialInfo);
		m_pMaterials.push_back(pMaterial);
	}
}

void DiffusedRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase)
{
	for (int i = 0; i < m_pMaterials.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), refnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, descHandle.gpuHandle);
		descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		// Texture (있다면)
		//m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMesh->Render(pd3dCommandList, i, nInstanceCount);
	}

	refnInstanceBase += nInstanceCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedRenderer

TexturedRenderer::TexturedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials)
{
}

TexturedRenderer::TexturedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{
	m_eObjectRenderType = OBJECT_RENDER_FORWARD;
	m_pMesh = std::make_shared<TexturedMesh>(meshLoadInfo);

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<TexturedMaterial> pMaterial = std::make_shared<TexturedMaterial>(materialInfo);
		m_pMaterials.push_back(pMaterial);
	}
}

void TexturedRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase)
{
	for (int i = 0; i < m_pMaterials.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), refnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, descHandle.gpuHandle);
		descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMesh->Render(pd3dCommandList, i, nInstanceCount);
	}

	refnInstanceBase += nInstanceCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedIlluminatedRenderer

TexturedIlluminatedRenderer::TexturedIlluminatedRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials)
{
}

TexturedIlluminatedRenderer::TexturedIlluminatedRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{
	m_eObjectRenderType = OBJECT_RENDER_FORWARD;
	m_pMesh = std::make_shared<TexturedNormalMesh>(meshLoadInfo);

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<TexturedMaterial> pMaterial = std::make_shared<TexturedMaterial>(materialInfo);
		m_pMaterials.push_back(pMaterial);
	}
}

void TexturedIlluminatedRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase)
{
	for (int i = 0; i < m_pMaterials.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), refnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);
		pd3dCommandList->SetGraphicsRootDescriptorTable(4, descHandle.gpuHandle);
		descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMesh->Render(pd3dCommandList, i, nInstanceCount);
	}

	refnInstanceBase += nInstanceCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TexturedNormalRenderer

TexturedNormalRenderer::TexturedNormalRenderer(std::shared_ptr<Mesh> pMesh, std::vector<std::shared_ptr<Material>> pMaterials)
{
}

TexturedNormalRenderer::TexturedNormalRenderer(const MESHLOADINFO& meshLoadInfo, const std::vector<MATERIALLOADINFO>& materialLoadInfo)
{
	m_eObjectRenderType = OBJECT_RENDER_FORWARD;
	m_pMesh = std::make_shared<TexturedNormalTangentMesh>(meshLoadInfo);

	for (const auto& materialInfo : materialLoadInfo) {
		std::shared_ptr<TexturedNormalMaterial> pMaterial = std::make_shared<TexturedNormalMaterial>(materialInfo);
		m_pMaterials.push_back(pMaterial);
	}
}

void TexturedNormalRenderer::Render(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, DescriptorHandle& descHandle, int nInstanceCount, int& refnInstanceBase)
{
	for (int i = 0; i < m_pMaterials.size(); ++i) {
		// Per Object CB
		CB_PER_OBJECT_DATA cbData = { m_pMaterials[i]->GetMaterialColors(), refnInstanceBase };
		ConstantBuffer cbuffer = RESOURCE->AllocCBuffer<CB_PER_OBJECT_DATA>();
		cbuffer.WriteData(&cbData);

		pd3dDevice->CopyDescriptorsSimple(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, descHandle.cpuHandle, cbuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		descHandle.cpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		// 4
		pd3dCommandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_OBJ_MATERIAL_DATA, descHandle.gpuHandle);
		descHandle.gpuHandle.Offset(ConstantBufferSize<CB_PER_OBJECT_DATA>::nDescriptors, D3DCore::g_nCBVSRVDescriptorIncrementSize);

		// Texture (있다면)
		m_pMaterials[i]->UpdateShaderVariables(pd3dDevice, pd3dCommandList, descHandle);	// Texture 가 있다면 Descriptor 가 복사될 것이고 아니면 안될것

		const auto& pipelineStates = m_pMaterials[i]->GetShader()->GetPipelineStates();
		pd3dCommandList->SetPipelineState(pipelineStates[0].Get());

		m_pMesh->Render(pd3dCommandList, i, nInstanceCount);
	}

	refnInstanceBase += nInstanceCount;
}
