#include "pch.h"
#include "SpritePass.h"
#include "Sprite.h"


void SpritePass::Initialize()
{
	m_pSpriteQuadMesh = std::make_shared<QuadMesh>(0.f, 1.f);

	CreatePipelineState();
}

void SpritePass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void SpritePass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
	auto sprites = RENDER->GetSprites();
	auto unDescriptorInc = D3DCore::g_nCBVSRVDescriptorIncrementSize;
	constexpr auto rootParamPerDraw = std::to_underlying(ROOT_PARAMETER::PER_PASS_DATA);
	constexpr auto rootParamSpriteRect = std::to_underlying(ROOT_PARAMETER::SPRITE_DATA);

	auto blankSBuffer = RENDER->AllocSBuffer<MaterialData>(1);

	pd3dCommandList->SetPipelineState(m_pd3dPipelineState.Get());

	for (const auto& layer : std::views::reverse(sprites)) {
		if (layer.size() == 0) {
			continue;
		}

		auto bindHandle = outDescHandle.gpuHandle;
		auto copyHandle = outDescHandle.cpuHandle;

		// Set Empty material data to fits root parameter (NumDescriptors = 2)
		DEVICE->CopyDescriptorsSimple(1, copyHandle, blankSBuffer.SRVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		copyHandle.Offset(1, unDescriptorInc);

		std::vector<SpriteRect> rects;
		rects.reserve(layer.size());
		for (const auto& sprite : layer) {
			auto cpuHandle = sprite.texHandle.GetResource()->GetSRVHandle();
			DEVICE->CopyDescriptorsSimple(1, copyHandle, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			copyHandle.Offset(1, unDescriptorInc);

			rects.push_back(sprite.Rect);
		}

		// Set rect data
		auto rectSBuffer = RENDER->AllocSBuffer<SpriteRect>(layer.size());
		rectSBuffer.WriteData(rects);
		pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamSpriteRect, rectSBuffer.GPUAddress);

		pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamPerDraw, bindHandle);
		outDescHandle.cpuHandle.Offset(1 + layer.size(), unDescriptorInc);
		outDescHandle.gpuHandle.Offset(1 + layer.size(), unDescriptorInc);

		m_pSpriteQuadMesh->Render(pd3dCommandList, layer.size());
	}
}

void SpritePass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle) const
{
}

void SpritePass::CreatePipelineState()
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
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("SpriteVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("SpritePS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.RasterizerState.FrontCounterClockwise = true;
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;

		d3dPipelineDesc.InputLayout = inputLayoutDesc;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
