#include "pch.h"
#include "AtmosphericFogDetailPass.h"
#include "PostProcessingVolume.h"

void AtmosphericFogDetailPass::Initialize()
{
	CreatePipelineState();
}

void AtmosphericFogDetailPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const auto& volume = CUR_SCENE->GetPostProcessingVolume();
	m_bShouldRender = volume.GetFogParameters().fFogDetailStrength > 0.0f;
	if (!m_bShouldRender) {
		return;
	}

	auto pHDRInput = RENDER->GetHDRBuffer(1).GetResource();
	auto pHDROutput = std::static_pointer_cast<RenderTargetTexture>(RENDER->GetHDRBuffer(0).GetResource());
	auto pDepthBuffer = RENDER->GetDepthStencilBuffer().GetResource();
	pHDRInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	pHDROutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pDepthBuffer->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pHDROutput->GetRTVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, pHDRInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	constexpr uint32 rootParamHDR = std::to_underlying(ROOT_PARAMETER::HDR_RESULT);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamHDR, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);

	CB_FOG_DATA fogData = volume.GetFogCBData();
	auto fogCBuffer = RENDER->AllocCBuffer<CB_FOG_DATA>();
	fogCBuffer.WriteData(&fogData);
	constexpr auto rootParamFog = std::to_underlying(ROOT_PARAMETER::FOG_DATA);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamFog, fogCBuffer.GPUAddress);
}

void AtmosphericFogDetailPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_bShouldRender) {
		return;
	}

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	RENDER->GetQuadMesh()->Render(pd3dCommandList, 1);
}

void AtmosphericFogDetailPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_bShouldRender) {
		return;
	}

	auto pHDRInput = RENDER->GetHDRBuffer(1).GetResource();
	auto pHDROutput = RENDER->GetHDRBuffer(0).GetResource();
	pHDROutput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
	pHDRInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_COPY_DEST);
	pd3dCommandList->CopyResource(pHDRInput->GetResourcePtr().Get(), pHDROutput->GetResourcePtr().Get());
	pHDRInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, pHDRInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	constexpr uint32 rootParamHDR = std::to_underlying(ROOT_PARAMETER::HDR_RESULT);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamHDR, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);
}

void AtmosphericFogDetailPass::ShowDebugInfo()
{
	ImGui::Text("Atmospheric fog detail is controlled by Post Processing Volume.");
	ImGui::Text("Pass Status : %s", m_bShouldRender ? "Enabled" : "Disabled");
}

void AtmosphericFogDetailPass::CreatePipelineState()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("QuadVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("AtmosphericFogDetailPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
