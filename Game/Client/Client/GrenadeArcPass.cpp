#include "pch.h"
#include "GrenadeArcPass.h"

void GrenadeArcPass::Initialize()
{
	m_pd3dArcPSO = CreateLinePipelineState();
}

void GrenadeArcPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	output = RenderPassOutput{
		.pRenderTargets = input.pRenderTargets,
		.passResource = input.passResource
	};

	if (s_v3ArcVertices.empty()) {
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	auto pDSV = std::static_pointer_cast<DepthStencilTexture>(RENDER->GetDepthStencilBuffer().GetResource());
	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_READ);
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = pDSV->GetDSVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVCPUDescriptorHandle);
}

void GrenadeArcPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (s_v3ArcVertices.empty()) {
		return;
	}

	auto vertexBuffer = RENDER->AllocSBuffer<Vector3>(static_cast<uint32>(s_v3ArcVertices.size()));
	vertexBuffer.WriteData(s_v3ArcVertices);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexBuffer.GPUAddress;
	vertexBufferView.StrideInBytes = sizeof(Vector3);
	vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(Vector3) * s_v3ArcVertices.size());

	pd3dCommandList->SetPipelineState(m_pd3dArcPSO.Get());
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pd3dCommandList->DrawInstanced(static_cast<UINT>(s_v3ArcVertices.size()), 1, 0, 0);
}

void GrenadeArcPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

ComPtr<ID3D12PipelineState> GrenadeArcPass::CreateLinePipelineState()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElements = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.NumElements = static_cast<UINT>(d3dInputElements.size());
	inputLayoutDesc.pInputElementDescs = d3dInputElements.data();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("DebugLineVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("DebugLineYellowPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 카메라 빌보드 쿼드 — 양면
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	ComPtr<ID3D12PipelineState> pd3dPipelineState = nullptr;
	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	return pd3dPipelineState;
}
