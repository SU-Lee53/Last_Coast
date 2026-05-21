#include "pch.h"
#include "SSAOPass.h"

void SSAOPass::Initialize()
{
	CreatePipelineStates();
	CreateNoiseTexture();
}

void SSAOPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Set SSAO data and noise texture
	const auto pNoise = m_NoiseTexture.GetResource();

	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	constexpr auto rootParamSSAODataAndNoise = std::to_underlying(ROOT_PARAMETER::SSAO_DATA_AND_NOISE);

	auto cpuHandle = outDescHandle.cpuHandle;
	auto gpuHandle = outDescHandle.gpuHandle;

	const auto& volume = CUR_SCENE->GetPostProcessingVolume();

	auto bloomData = volume.GetSSAOCBData();
	auto cBuffer = RENDER->AllocCBuffer<CB_BLOOM_DATA>();
	cBuffer.WriteData(&bloomData);

	DEVICE->CopyDescriptorsSimple(1, cpuHandle, cBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DEVICE->CopyDescriptorsSimple(1, cpuHandle.Offset(1, nDescriptorInc), pNoise->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamSSAODataAndNoise, gpuHandle);
	outDescHandle.cpuHandle.Offset(2, nDescriptorInc);
	outDescHandle.gpuHandle.Offset(2, nDescriptorInc);

	// Set viewport and scissor rects
	//const DWORD nWidth = WinCore::g_dwClientWidth;
	//const DWORD nHeight = WinCore::g_dwClientHeight;
	//
	//D3D12_VIEWPORT d3dViewport{
	//	.TopLeftX = 0.f,
	//	.TopLeftY = 0.f,
	//	.Width = static_cast<float>(nWidth / 2),
	//	.Height = static_cast<float>(nHeight / 2),
	//	.MinDepth = 0.f,
	//	.MaxDepth = 1.f,
	//};
	//
	//D3D12_RECT d3dScissorRect{
	//	.left = 0,
	//	.top = 0,
	//	.right = static_cast<LONG>(nWidth / 2),
	//	.bottom = static_cast<LONG>(nHeight / 2),
	//};
	//
	//pd3dCommandList->RSSetViewports(1, &d3dViewport);
	//pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect);
}

void SSAOPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const auto pSSAOInput = RENDER->GetPostProcessingResources().SSAOBuffer.GetResource();
	const auto pSSAOBlur = RENDER->GetPostProcessingResources().SSAOBlurBuffer.GetResource();

	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	constexpr auto rootParamSSAOInput = std::to_underlying(ROOT_PARAMETER::SSAO_INPUT);
	constexpr auto rootParamSSAOBlur = std::to_underlying(ROOT_PARAMETER::SSAO_OUTPUT);

	auto cpuHandle = outDescHandle.cpuHandle;
	auto gpuHandle = outDescHandle.gpuHandle;

	auto pQuadMesh = RENDER->GetQuadMesh();

	// SSAO
	pd3dCommandList->SetPipelineState(m_pd3dSSAOPipelineState.Get());

	pSSAOInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	float pfClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoInputRTVHandle = pSSAOInput->GetRTVHandle();
	pd3dCommandList->ClearRenderTargetView(ssaoInputRTVHandle, pfClearColor, 0, nullptr);
	pd3dCommandList->OMSetRenderTargets(1, &ssaoInputRTVHandle, true, nullptr);
	// ...
	pQuadMesh->Render(pd3dCommandList, 1);

	// Set SSAO Input
	pSSAOInput->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	DEVICE->CopyDescriptorsSimple(1, cpuHandle, pSSAOInput->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamSSAOInput, gpuHandle);
	outDescHandle.cpuHandle.Offset(1, nDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, nDescriptorInc);

	// SSAO Blur
	pd3dCommandList->SetPipelineState(m_pd3dSSAOBlurPipelineState.Get());
	pSSAOBlur->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_CPU_DESCRIPTOR_HANDLE ssaoBlurRTVHandle = pSSAOBlur->GetRTVHandle();
	pd3dCommandList->ClearRenderTargetView(ssaoBlurRTVHandle, pfClearColor, 0, nullptr);
	pd3dCommandList->OMSetRenderTargets(1, &ssaoBlurRTVHandle, true, nullptr);
	// ...
	pQuadMesh->Render(pd3dCommandList, 1);
}

void SSAOPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Set SSAO Blur
	const auto pSSAOBlur = RENDER->GetPostProcessingResources().SSAOBlurBuffer.GetResource();
	pSSAOBlur->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	auto nDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	constexpr auto rootParamSSAOBlur = std::to_underlying(ROOT_PARAMETER::SSAO_OUTPUT);

	auto cpuHandle = outDescHandle.cpuHandle;
	auto gpuHandle = outDescHandle.gpuHandle;

	DEVICE->CopyDescriptorsSimple(1, cpuHandle, pSSAOBlur->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamSSAOBlur, gpuHandle);
	outDescHandle.cpuHandle.Offset(1, nDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, nDescriptorInc);

	// Restore viewport & scissor rects
	//auto pCamera = CUR_SCENE->GetCamera();
	//pCamera->SetViewportsAndScissorRects(pd3dCommandList);
}

void SSAOPass::CreateNoiseTexture()
{
	std::uniform_real_distribution<float> uid{ -1.f, 1.f};
	std::vector<Vector4> noiseData;
	noiseData.reserve(16);

	for (int i = 0; i < 16; ++i)
	{
		auto& rng = RandomGenerator::g_dre;
		Vector3 v(uid(rng), uid(rng), 0.0f);
		v.Normalize();

		noiseData.emplace_back(v.x, v.y, 0.0f, 0.0f);
	}

	m_NoiseTexture = TEXTURE->LoadTextureFromRawData("SSAO_Noise", noiseData, 4, 4, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void SSAOPass::CreatePipelineStates()
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
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("SSAOPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;

		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dSSAOPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	{
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("SSAOBilateralBlurPS");
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dSSAOBlurPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
