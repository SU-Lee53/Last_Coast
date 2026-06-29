#include "pch.h"
#include "BoundingBoxDebugPass.h"

void BoundingBoxDebugPass::Initialize()
{
	CreatePipelineState();
}

void BoundingBoxDebugPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	output = RenderPassOutput{
		.pRenderTargets = input.pRenderTargets,
		.passResource = input.passResource
	};

	m_v3LineVertices.clear();
	m_unDrawnColliders = 0;

	if (!m_bEnabled) {
		return;
	}

	BuildCollisionLineVertices();

	if (m_v3LineVertices.empty()) {
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	auto pDSV = std::static_pointer_cast<DepthStencilTexture>(RENDER->GetDepthStencilBuffer().GetResource());
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = pDSV->GetDSVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVCPUDescriptorHandle);
}

void BoundingBoxDebugPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_bEnabled || m_v3LineVertices.empty()) {
		return;
	}

	auto vertexBuffer = RENDER->AllocSBuffer<Vector3>(static_cast<uint32>(m_v3LineVertices.size()));
	vertexBuffer.WriteData(m_v3LineVertices);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexBuffer.GPUAddress;
	vertexBufferView.StrideInBytes = sizeof(Vector3);
	vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(Vector3) * m_v3LineVertices.size());

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	pd3dCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pd3dCommandList->DrawInstanced(static_cast<UINT>(m_v3LineVertices.size()), 1, 0, 0);
}

void BoundingBoxDebugPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

void BoundingBoxDebugPass::ShowDebugInfo()
{
	ImGui::Checkbox("Draw root collision bounds", &m_bEnabled);
	ImGui::Text("Drawn root colliders: %u", m_unDrawnColliders);
	ImGui::Text("Line vertices: %zu", m_v3LineVertices.size());
}

void BoundingBoxDebugPass::AppendBoundingBoxLines(const BoundingBox& xmAABB)
{
	XMFLOAT3 corners[BoundingBox::CORNER_COUNT];
	xmAABB.GetCorners(corners);

	constexpr uint32 edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};

	for (const auto& edge : edges) {
		m_v3LineVertices.emplace_back(corners[edge[0]]);
		m_v3LineVertices.emplace_back(corners[edge[1]]);
	}
}

void BoundingBoxDebugPass::AppendBoundingBoxLines(const BoundingOrientedBox& xmOBB)
{
	XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
	xmOBB.GetCorners(corners);

	constexpr uint32 edges[][2] = {
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};

	for (const auto& edge : edges) {
		m_v3LineVertices.emplace_back(corners[edge[0]]);
		m_v3LineVertices.emplace_back(corners[edge[1]]);
	}
}

void BoundingBoxDebugPass::BuildCollisionLineVertices()
{
	const auto& world = CUR_SCENE->GetWorld();
	m_v3LineVertices.reserve((world.SizeAll() + 1) * 24);

	auto appendColliderBounds = [this](const auto& pObj) {
		if (!pObj) {
			return;
		}

		const auto pCollider = pObj->GetComponent<ICollider>();
		if (!pCollider) {
			return;
		}

		if (std::dynamic_pointer_cast<PlayerCollider>(pCollider)) {
			AppendBoundingBoxLines(pCollider->GetAABBFromOBBWorld());
		}
		else {
			AppendBoundingBoxLines(pCollider->GetOBBWorld());
		}
		++m_unDrawnColliders;
	};

	appendColliderBounds(CUR_SCENE->GetPlayer());

	world.ForEachAliveAll([&appendColliderBounds](const auto& pObj) {
		appendColliderBounds(pObj);
	});
}

void BoundingBoxDebugPass::CreatePipelineState()
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
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("DebugLineGreenPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
