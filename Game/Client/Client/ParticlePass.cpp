#include "pch.h"
#include "ParticlePass.h"
#include "BloomPass.h"

void ParticlePass::Initialize()
{
	CreatePipelineStates();
}

void ParticlePass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	BuildDrawUnits();
	SetRenderTargets(pd3dCommandList);
}

void ParticlePass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	if (!m_AdditiveDrawUnit.drawDatas.empty()) {
		pd3dCommandList->SetPipelineState(m_pd3dAdditivePipelineState.Get());
		DrawUnit(pd3dCommandList, m_AdditiveDrawUnit, outDescHandle);
	}

	if (!m_AlphaDrawUnit.drawDatas.empty()) {
		pd3dCommandList->SetPipelineState(m_pd3dBlendPipelineState.Get());
		DrawUnit(pd3dCommandList, m_AlphaDrawUnit, outDescHandle);
	}
}

void ParticlePass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	const uint32 unCurrentContext = RENDER->GetCurrentContextIndex();
	auto pRTV = RENDER->GetHDRBuffer(1).GetResource();
	pRTV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	// Set HDR result
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE hdrHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamHDR = std::to_underlying(ROOT_PARAMETER::HDR_RESULT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVHandle = pRTV->GetSRVHandle();
	DEVICE->CopyDescriptorsSimple(1, hdrHandle, rtSRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	hdrHandle.Offset(1, unDescriptorInc);

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamHDR, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1, unDescriptorInc);
}

void ParticlePass::ShowDebugInfo()
{
	ImGui::Text("Additive particle counts : %d", m_unAdditiveParticles);
	ImGui::Text("Alpha particle counts : %d", m_unAlphaParticles);
}

void ParticlePass::SetRenderTargets(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList) const
{
	auto pRTV = static_pointer_cast<RenderTargetTexture>(RENDER->GetHDRBuffer(1).GetResource());
	auto pDSV = static_pointer_cast<DepthStencilTexture>(RENDER->GetDepthStencilBuffer().GetResource());

	pDSV->StateTransition(pd3dCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);			// 이전 G-Buffer Pass 에서 ALL_SHADER_RESOURCE 로 바꾸었으므로 한번 전환이 필요함
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = pRTV->GetRTVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dDSVCPUDescriptorHandle = pDSV->GetDSVHandle();

	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, &d3dDSVCPUDescriptorHandle);
}

void ParticlePass::BuildDrawUnits()
{
	m_AdditiveDrawUnit.Clear();
	m_AlphaDrawUnit.Clear();
	m_AlphaParticles.clear();

	m_AdditiveDrawUnit.eBlendMode = PARTICLE_BLEND_MODE::ADDITIVE;
	m_AlphaDrawUnit.eBlendMode = PARTICLE_BLEND_MODE::ALPHA_BLEND;

	m_unAdditiveParticles = 0;
	m_unAlphaParticles = 0;

	const auto& batches = PARTICLE->GetRenderBatches();
	if (batches.size() == 0) {
		return;
	}

	size_t estimatedParticles = 0;
	size_t estimatedTextures = 0;

	for (const auto& batch : batches) {
		estimatedParticles += batch.drawDatas.size();

		if (!batch.drawDatas.empty() && batch.textureRef.IsValid()) {
			++estimatedTextures;
		}
	}

	m_AdditiveDrawUnit.Reserve(estimatedTextures, estimatedParticles);
	m_AlphaDrawUnit.Reserve(estimatedTextures, estimatedParticles);
	m_AlphaParticles.reserve(estimatedParticles);

	BuildAdditiveDrawUnit(batches);
	BuildAlphaDrawUnit(batches);

	m_unAdditiveParticles = static_cast<uint32>(m_AdditiveDrawUnit.drawDatas.size());
	m_unAlphaParticles = static_cast<uint32>(m_AlphaDrawUnit.drawDatas.size());
}


void ParticlePass::BuildAdditiveDrawUnit(const std::vector<ParticleRenderBatch>& batches)
{
	for (const ParticleRenderBatch& batch : batches) {
		if (batch.drawDatas.empty()) {
			continue;
		}

		if (batch.eBlendMode != PARTICLE_BLEND_MODE::ADDITIVE) {
			continue;
		}

		if (!batch.textureRef.IsValid()) {
			continue;
		}

		AppendBatchToDrawUnit(m_AdditiveDrawUnit, batch);
	}
}

void ParticlePass::BuildAlphaDrawUnit(const std::vector<ParticleRenderBatch>& batches)
{
	const Vector3 v3CameraPos = CUR_SCENE->GetCamera()->GetPosition();

	for (const ParticleRenderBatch& batch : batches) {
		if (batch.drawDatas.empty()) {
			continue;
		}

		if (batch.eBlendMode != PARTICLE_BLEND_MODE::ALPHA_BLEND) {
			continue;
		}

		if (!batch.textureRef.IsValid()) {
			continue;
		}

		for (const ParticleDrawData& drawData : batch.drawDatas) {
			AlphaParticle alphaParticle{};
			alphaParticle.textureRef = &batch.textureRef;
			alphaParticle.drawData = drawData;
			alphaParticle.fDistanceSq = Vector3::DistanceSquared(
				drawData.v3Position,
				v3CameraPos
			);

			m_AlphaParticles.emplace_back(std::move(alphaParticle));
		}
	}

	std::sort(
		m_AlphaParticles.begin(),
		m_AlphaParticles.end(),
		[](const AlphaParticle& lhs, const AlphaParticle& rhs) {
			return lhs.fDistanceSq > rhs.fDistanceSq;
		}
	);

	m_AlphaDrawUnit.drawDatas.reserve(m_AlphaParticles.size());

	for (const AlphaParticle& alphaParticle : m_AlphaParticles) {
		ParticleDrawData drawData = alphaParticle.drawData;

		const uint32 textureIndex = InsertTextureAndGetIndex(
			m_AlphaDrawUnit,
			*(alphaParticle.textureRef)
		);

		drawData.nTextureIndex = static_cast<int32>(textureIndex);
		m_AlphaDrawUnit.drawDatas.emplace_back(drawData);
	}
}

void ParticlePass::AppendBatchToDrawUnit(ParticleDrawUnit& drawUnit, const ParticleRenderBatch& batch)
{
	const uint32 textureIndex = InsertTextureAndGetIndex(
		drawUnit,
		batch.textureRef
	);

	drawUnit.drawDatas.reserve(
		drawUnit.drawDatas.size() + batch.drawDatas.size()
	);

	for (const ParticleDrawData& srcDrawData : batch.drawDatas) {
		ParticleDrawData drawData = srcDrawData;
		drawData.nTextureIndex = static_cast<int32>(textureIndex);

		drawUnit.drawDatas.emplace_back(drawData);
	}
}

uint32 ParticlePass::InsertTextureAndGetIndex(ParticleDrawUnit& drawUnit, const TextureRef<Texture>& textureRef)
{
	const Texture::ID textureID = textureRef.GetID();

	auto [idx, bInserted] = drawUnit.textureMap.Insert(textureID, &textureRef);

	return static_cast<uint32>(idx);
}

void ParticlePass::DrawUnit(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const ParticleDrawUnit& drawUnit, OUT DescriptorHandle& outDescHandle)
{
	if (drawUnit.drawDatas.empty()) {
		return;
	}

	if (drawUnit.textureMap.Size() == 0) {
		return;
	}

	BindParticleDrawUnit(
		pd3dCommandList,
		drawUnit,
		outDescHandle
	);

	auto pQuadMesh = RENDER->GetQuadMesh();
	pQuadMesh->Render(
		pd3dCommandList,
		static_cast<uint32>(drawUnit.drawDatas.size())
	);
}

void ParticlePass::BindParticleDrawUnit(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const ParticleDrawUnit& drawUnit, OUT DescriptorHandle& outDescHandle)
{
	constexpr uint32 rootParamParticleDatas = std::to_underlying(ROOT_PARAMETER::PARTICLE_DATA);
	constexpr uint32 rootParamTextures = std::to_underlying(ROOT_PARAMETER::PER_PASS_TEXTURES);
	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;

	// ParticleDrawData : RootParam[16]
	const auto& drawDatas = drawUnit.drawDatas;
	auto particleSBuffer = RENDER->AllocSBuffer<ParticleDrawData>(drawDatas.size());
	particleSBuffer.WriteData(drawDatas);
	pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamParticleDatas, particleSBuffer.GPUAddress);

	// Textures : RootParam[8]
	const auto& texDatas = drawUnit.textureMap.GetElements();
	const uint32 unNumTextures = texDatas.size();
	for (const auto& pTexHandle : texDatas) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE texCPUHandle = pTexHandle->GetResource()->GetSRVHandle();
		DEVICE->CopyDescriptorsSimple(1, bindHandle, texCPUHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		bindHandle.Offset(1, unDescriptorInc);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTextures, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(unNumTextures, unDescriptorInc);
	outDescHandle.gpuHandle.Offset(unNumTextures, unDescriptorInc);
}

void ParticlePass::CreatePipelineStates()
{
	std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputElements = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.NumElements = d3dInputElements.size();
	inputLayoutDesc.pInputElementDescs = d3dInputElements.data();

	// 1. PARTICLE_BLEND_MODE::ADDITIVE
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = RenderManager::g_pd3dGlobalRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("ParticleVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("ParticlePS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
		d3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;

		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dAdditivePipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 2. PARTICLE_BLEND_MODE::ALPHA_BLEND
	{
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dBlendPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
