#include "pch.h"
#include "DeferredFogPass.h"
#include "PostProcessingVolume.h"

void DeferredFogPass::Initialize()
{
	CreatePipelineState();
}

void DeferredFogPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Update Height Fog base height

	// Set Render Targets
	auto pHDRRenderTarget = std::static_pointer_cast<RenderTargetTexture>(RENDER->GetHDRBuffer(1).GetResource());
	pHDRRenderTarget->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Clear Render Targets
	float pfClearColor[4] = { 0.f, 0.0f, 0.0f, 1.0f };

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pHDRRenderTarget->GetRTVHandle();
	pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, NULL);

	//CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVDescriptorHandle = RENDER->GetDepthStencilBufferHandle();
	//pd3dCommandList->ClearDepthStencilView(d3dDSVDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, NULL);

	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);
}

void DeferredFogPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	constexpr auto rootParamFog = std::to_underlying(ROOT_PARAMETER::FOG_DATA);
	
	const auto& volume = CUR_SCENE->GetPostProcessingVolume();
	CB_FOG_DATA fogData = volume.GetFogCBData();
	auto fogCBuffer = RENDER->AllocCBuffer<CB_FOG_DATA>();
	fogCBuffer.WriteData(&fogData);
	pd3dCommandList->SetGraphicsRootConstantBufferView(rootParamFog, fogCBuffer.GPUAddress);

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(pd3dCommandList, 1);
}

void DeferredFogPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

void DeferredFogPass::ShowDebugInfo()
{
	ImGui::Text("Fog parameters are controlled by Post Processing Volume.");
}

void DeferredFogPass::CreatePipelineState()
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
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("DefferedFogPS");
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

