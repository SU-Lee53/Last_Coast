#include "pch.h"
#include "LightShaftPass.h"
#include "Skybox.h"

void LightShaftPass::Initialize()
{
	CreatePipelineState();
}

void LightShaftPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const auto pLightShaft = RENDER->GetPostProcessingResources().LightShaftBuffer.GetResource();
	pLightShaft->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto pDSV = RENDER->GetDepthStencilBuffer().GetResource();
	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	float pfClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pLightShaft->GetRTVHandle();
	pd3dCommandList->ClearRenderTargetView(d3dRTVCPUDescriptorHandle, pfClearColor, 0, nullptr);
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, true, nullptr);

	constexpr auto rootParamLightShaftData = std::to_underlying(ROOT_PARAMETER::LIGHT_SHAFT_DATA);
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	auto cbLightShaftData = MakeLightShaftCBData();
	m_bShouldRender =
		cbLightShaftData.gnEnable != 0 &&
		cbLightShaftData.gnSampleCount > 0 &&
		cbLightShaftData.gfIntensity > 0.0f &&
		cbLightShaftData.gfWeight > 0.0f &&
		cbLightShaftData.gfExposure > 0.0f;

	auto cBuffer = RENDER->AllocCBuffer<CB_LIGHT_SHAFT_DATA>();
	cBuffer.WriteData(&cbLightShaftData);

	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, cBuffer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamLightShaftData, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);
}

void LightShaftPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_bShouldRender) {
		return;
	}

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(pd3dCommandList, 1);
}

void LightShaftPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const auto pLightShaft = RENDER->GetPostProcessingResources().LightShaftBuffer.GetResource();
	pLightShaft->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	constexpr auto rootParamLightShaftResult = std::to_underlying(ROOT_PARAMETER::LIGHT_SHAFT_RESULT);
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);

	DEVICE->CopyDescriptorsSimple(1, outDescHandle.cpuHandle, pLightShaft->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamLightShaftResult, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);
}

void LightShaftPass::ShowDebugInfo()
{
	ImGui::Text("Light in front : %s", (m_bLightInFront) ? "TRUE" : "FALSE");
	ImGui::Text("Render enabled : %s", (m_bShouldRender) ? "TRUE" : "FALSE");
	ImGui::Text("Light Screen Position : %f, %f", m_v2LightScreenPosition.x, m_v2LightScreenPosition.y);
}

void LightShaftPass::CreatePipelineState()
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
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("LightShaftPS");
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

CB_LIGHT_SHAFT_DATA LightShaftPass::MakeLightShaftCBData()
{
	Vector3 v3SunDirection = Vector3::Zero;

	if (const auto& pSkybox = CUR_SCENE->GetSkybox()) {
		v3SunDirection = pSkybox->MakeCBData().v3SunDirection;
	}

	if (v3SunDirection.LengthSquared() <= 1e-6f) {
		if (const auto pDirectionalLight = CUR_SCENE->GetSunLight()) {
			v3SunDirection = -pDirectionalLight->m_v3Direction;
		}
	}

	if (v3SunDirection.LengthSquared() <= 1e-6f) {
		v3SunDirection = CUR_SCENE->GetCamera()->GetLook();
	}

	v3SunDirection.Normalize();

	const auto pCamera = CUR_SCENE->GetCamera();
	const Vector3 v3SunWorldPosition = pCamera->GetPosition() + v3SunDirection * pCamera->GetFarPlaneDistance() * 0.5f;

	Vector4 v4LightClipPosition;
	Vector3::Transform(v3SunWorldPosition, pCamera->GetViewProjectMatrix(), v4LightClipPosition);

	m_bLightInFront = v4LightClipPosition.w > 0.0f;

	if (std::abs(v4LightClipPosition.w) > 1e-6f) {
		const float fInvW = 1.0f / v4LightClipPosition.w;
		const float fNdcX = v4LightClipPosition.x * fInvW;
		const float fNdcY = v4LightClipPosition.y * fInvW;

		m_v2LightScreenPosition.x = fNdcX * 0.5f + 0.5f;
		m_v2LightScreenPosition.y = 0.5f - fNdcY * 0.5f;
	}

	const auto& volume = CUR_SCENE->GetPostProcessingVolume();
	CB_LIGHT_SHAFT_DATA cbData = volume.GetLightShaftCBData(m_v2LightScreenPosition);
	if (!m_bLightInFront) {
		cbData.gnEnable = 0;
	}

	return cbData;
}
