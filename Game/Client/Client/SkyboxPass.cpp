#include "pch.h"
#include "SkyboxPass.h"

void SkyboxPass::Initialize()
{
	m_pCubeMesh = std::make_shared<CubeMesh>(Vector3(1, 1, 1));
	CreatePipelineState();
}

void SkyboxPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	auto pRTV = input.pRenderTargets[0];	// 이전 DefferedLightingPass 에서 사용한 R16G16B16A16_FLOAT 리소스를 넘겨받아야 함
	auto pDSV = RENDER->GetDepthStencilBuffer();

	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);			// 이전 G-Buffer Pass 에서 ALL_SHADER_RESOURCE 로 바꾸었으므로 한번 전환이 필요함
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pRTV->GetRTVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = pDSV->GetDSVHandle();

	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVCPUDescriptorHandle);
}

void SkyboxPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	m_pCubeMesh->Render(pd3dCommandList, 1);
}

void SkyboxPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();
	input.pRenderTargets[0]->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	// Set HDR result
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE hdrHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamHDR = std::to_underlying(ROOT_PARAMETER::HDR_RESULT);

	uint32 unNumHDRResults = input.pRenderTargets.size();
	for (uint32 i = 0; i < unNumHDRResults; ++i) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVHandle = input.pRenderTargets[i]->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, hdrHandle, rtSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		hdrHandle.Offset(1, unDescriptorInc);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamHDR, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(unNumHDRResults, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(unNumHDRResults, unDescriptorInc);
}

void SkyboxPass::CreatePipelineState()
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
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("SkyboxVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("SkyboxPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = false;
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
