#include "pch.h"
#include "NavMeshDebugPass.h"

void NavMeshDebugPass::Initialize()
{
	CreatePipelineStates();
}

void NavMeshDebugPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	output = RenderPassOutput{
		.pRenderTargets = input.pRenderTargets,
		.passResource = input.passResource
	};

	if (!m_bEnabled) {
		return;
	}

	if (!m_bBuilt) {
		BuildNavMeshLineVertices();
	}

	if (m_v3PolygonEdgeVertices.empty() && m_v3AdjacencyVertices.empty()) {
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	auto pDSV = std::static_pointer_cast<DepthStencilTexture>(RENDER->GetDepthStencilBuffer().GetResource());
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = pDSV->GetDSVHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVCPUDescriptorHandle);
}

void NavMeshDebugPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_bEnabled || !m_bBuilt) {
		return;
	}

	if (m_bShowPolygonEdges) {
		DrawLines(pd3dCommandList, m_pd3dPolygonEdgePSO.Get(), m_v3PolygonEdgeVertices);
	}

	if (m_bShowAdjacency) {
		DrawLines(pd3dCommandList, m_pd3dAdjacencyPSO.Get(), m_v3AdjacencyVertices);
	}
}

void NavMeshDebugPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
}

void NavMeshDebugPass::ShowDebugInfo()
{
	ImGui::Checkbox("Draw NavMesh", &m_bEnabled);
	ImGui::Checkbox("Polygon edges (green)", &m_bShowPolygonEdges);
	ImGui::Checkbox("Adjacency graph (red)", &m_bShowAdjacency);

	if (ImGui::SliderFloat("Height offset (cm)", &m_fHeightOffset, 0.0f, 50.0f)) {
		m_bBuilt = false;
	}
	if (ImGui::Button("Rebuild")) {
		m_bBuilt = false;
	}

	ImGui::Text("Polygon edge vertices: %zu", m_v3PolygonEdgeVertices.size());
	ImGui::Text("Adjacency vertices: %zu", m_v3AdjacencyVertices.size());
}

void NavMeshDebugPass::BuildNavMeshLineVertices()
{
	m_v3PolygonEdgeVertices.clear();
	m_v3AdjacencyVertices.clear();

	if (!AI->IsInitialized()) {
		return;
	}

	auto pNavMesh = AI->GetNavMesh();
	if (!pNavMesh) {
		return;
	}

	NavMeshDebugData debugData = pNavMesh->GetDebugLineVertices();
	m_v3PolygonEdgeVertices = std::move(debugData.PolygonEdges);
	m_v3AdjacencyVertices = std::move(debugData.AdjacencyEdges);

	for (auto& v3Vertex : m_v3PolygonEdgeVertices) {
		v3Vertex.y += m_fHeightOffset;
	}
	for (auto& v3Vertex : m_v3AdjacencyVertices) {
		v3Vertex.y += m_fHeightOffset;
	}

	m_bBuilt = true;
}

void NavMeshDebugPass::DrawLines(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, ID3D12PipelineState* pd3dPipelineState, const std::vector<Vector3>& v3Vertices)
{
	if (v3Vertices.empty()) {
		return;
	}

	auto vertexBuffer = RENDER->AllocSBuffer<Vector3>(static_cast<uint32>(v3Vertices.size()));
	vertexBuffer.WriteData(v3Vertices);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexBuffer.GPUAddress;
	vertexBufferView.StrideInBytes = sizeof(Vector3);
	vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(Vector3) * v3Vertices.size());

	pd3dCommandList->SetPipelineState(pd3dPipelineState);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	pd3dCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pd3dCommandList->DrawInstanced(static_cast<UINT>(v3Vertices.size()), 1, 0, 0);
}

void NavMeshDebugPass::CreatePipelineStates()
{
	m_pd3dPolygonEdgePSO = CreateLinePipelineState("DebugLineGreenPS");
	m_pd3dAdjacencyPSO = CreateLinePipelineState("DebugLineRedPS");
}

ComPtr<ID3D12PipelineState> NavMeshDebugPass::CreateLinePipelineState(const std::string& strPSName)
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
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode(strPSName);
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

	ComPtr<ID3D12PipelineState> pd3dPipelineState = nullptr;
	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	return pd3dPipelineState;
}
