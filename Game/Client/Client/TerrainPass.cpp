#include "pch.h"
#include "TerrainPass.h"
#include "TerrainObject.h"

void IRenderPass::Execute(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	OnPreRender(pd3dCommandList, input, output, outDescHandle);
	Render(pd3dCommandList, input, output, outDescHandle);
	OnPostRender(pd3dCommandList, input, output, outDescHandle);
}

void TerrainPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;
	const uint32 rootParamTerrainLayer = std::to_underlying(ROOT_PARAMETER::TERRAIN_LAYER);

	// cbTerrainLayerData
	CB_TERRAIN_LAYER_DATA cbData = pTerrain->MakeLayerCBData();
	ConstantBuffer cbTerrainLayer = RENDER->AllocCBuffer<CB_TERRAIN_LAYER_DATA>();
	cbTerrainLayer.WriteData(&cbData);

	CD3DX12_CPU_DESCRIPTOR_HANDLE terrainLayerHandle = outDescHandle.cpuHandle;
	DEVICE->CopyDescriptorsSimple(1, terrainLayerHandle, cbTerrainLayer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	terrainLayerHandle.Offset(1, unDescriptorInc);

	// gtxtTerrainAlbedo[4]
	// gtxtTerrainNormal[4]
	CD3DX12_CPU_DESCRIPTOR_HANDLE albedoHandleBase = terrainLayerHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalHandleBase = terrainLayerHandle;
	normalHandleBase.Offset(4, unDescriptorInc);

	const auto& terrainMaterialIDs = pTerrain->GetComponent<MeshRenderer>()->GetMaterialIDs();
	for (int i = 0; i < 4; ++i) {
		if (i < terrainMaterialIDs.size()) {
			if (terrainMaterialIDs[i] != INVALID_ID) {
				auto& pMaterial = MATERIAL->GetMaterialByID(terrainMaterialIDs[i]);
				auto& pAlbedoTex = pMaterial->GetTexture(0);	// Albedo
				auto& pNormalTex = pMaterial->GetTexture(1);	// Normal

				DEVICE->CopyDescriptorsSimple(1, albedoHandleBase, pAlbedoTex->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				DEVICE->CopyDescriptorsSimple(1, normalHandleBase, pNormalTex->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}

		albedoHandleBase.Offset(1, unDescriptorInc);
		normalHandleBase.Offset(1, unDescriptorInc);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTerrainLayer, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(9, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(9, unDescriptorInc);

}

void TerrainPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const auto& pTerrain = CUR_SCENE->GetTerrain();
	const auto& pTerrainComponents = pTerrain->GetTerrainComponents();
	const auto& pTerrainMesh = pTerrain->GetComponent<MeshRenderer>()->GetMeshes()[0];

	const uint32 rootParamWorldTransform = std::to_underlying(ROOT_PARAMETER::WORLD_TRANSFORM_DATA);
	const uint32 rootParamTerrainComponent = std::to_underlying(ROOT_PARAMETER::TERRAIN_COMPONENT_AND_WEIGHTMAP);
	const uint32 unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;

	auto pd3dTerrainPipelineState = SHADER->Get<TerrainShader>()->GetPipelineStates()[0];
	pd3dCommandList->SetPipelineState(pd3dTerrainPipelineState.Get());

	StructuredBuffer sbWorldTransforms = RENDER->AllocSBuffer<Matrix>(1);
	sbWorldTransforms.WriteData(pTerrain->GetWorldMatrix().Transpose(), 0);
	pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamWorldTransform, sbWorldTransforms.GPUAddress);

	for (const auto& pComponent : pTerrainComponents) {
		// Component
		CB_TERRAIN_COMPONENT_DATA cbData = pComponent->MakeCBData();
		ConstantBuffer cbTerrainComponents = RENDER->AllocCBuffer<CB_TERRAIN_COMPONENT_DATA>();
		cbTerrainComponents.WriteData(&cbData);

		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, cbTerrainComponents.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		// Texture
		Texture::ID weightMapID = pComponent->GetWeightMapID();
		CD3DX12_CPU_DESCRIPTOR_HANDLE weightMapHandle = TEXTURE->GetTextureByID(weightMapID, TEXTURE_RESOURCE_TYPE::SRV)->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, weightMapHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		outDescHandle.cpuHandle.Offset(1, unDescriptorInc);

		pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTerrainComponent, outDescHandle.gpuHandle);
		outDescHandle.gpuHandle.Offset(2, unDescriptorInc);

		const auto& terrainIndexRange = pComponent->GetIndexRange();
		pTerrainMesh->Render(pd3dCommandList, 1, terrainIndexRange.unStartIndex, terrainIndexRange.unIndexCount);
	}

}

void TerrainPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}
