#include "pch.h"
#include "UIPass.h"
#include "UIBoard.h"
#include "TextBox.h"
#include "Sprite.h"

void UIPass::Initialize()
{
	CreatePipelineState();
}

void UIPass::OnPreRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	m_CachedData.Clear();
	if (!CUR_SCENE->GetUIBoard()) {
		return;
	}

	const UIBoard::UIComponentLayer& spriteLayers = CUR_SCENE->GetUIBoard()->GetSpriteLayers();
	const UIBoard::UIComponentLayer& textLayers = CUR_SCENE->GetUIBoard()->GetTextLayers();

	const uint32 unDescriptorInc = D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE::CBV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE bindHandle = outDescHandle.cpuHandle;
	constexpr uint32 rootParamTextures = std::to_underlying(ROOT_PARAMETER::PER_PASS_TEXTURES);

	const auto& atlasRef = RENDER->GetTextRenderer().GetAtlasTextureRef();
	auto pAtlas = atlasRef.GetResource();
	DEVICE->CopyDescriptorsSimple(1, bindHandle, pAtlas->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// gtxtTextures[0] -> TextAtlas

	auto nLayers = UIBoard::g_unUILayers;
	for (int i = nLayers - 1; i >= 0; --i) {
		const auto& textsInLayer = textLayers[i];
		const auto& spritesInLayer = spriteLayers[i];

		for (auto& pText : textsInLayer | std::views::transform([](const auto& p) { return std::static_pointer_cast<TextBox>(p); })) {
			m_CachedData.textDatas[i].push_back(pText->MakeSBData());
		}

		for (auto& pSprites : spritesInLayer | std::views::transform([](const auto& p) { return std::static_pointer_cast<Sprite>(p); })) {
			if (!pSprites->IsVisible()) {
				continue;
			}
			auto UIData = pSprites->MakeSBData();
			const auto& texRef = pSprites->GetTextureRef();

			auto [id, bInserted] = m_CachedData.textureMap.Insert(texRef.GetID(), &texRef);

			UIData.v4TextColorOrTexIndex.x = static_cast<float>(id + 1);	// 0번에 Atlas 가 보관되므로 +1 index 에 원하는 Texture 가 바인딩됨

			m_CachedData.spriteDatas[i].push_back(UIData);
		}
	}

	for (const auto& pTex : m_CachedData.textureMap.GetElements() | std::views::transform([](const auto& ref) { return ref->GetResource(); })) {
		DEVICE->CopyDescriptorsSimple(1, bindHandle.Offset(1, unDescriptorInc), pTex->GetSRVHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	pd3dCommandList->SetGraphicsRootDescriptorTable(rootParamTextures, outDescHandle.gpuHandle);
	outDescHandle.cpuHandle.Offset(1 + m_CachedData.textureMap.Size(), unDescriptorInc);
	outDescHandle.gpuHandle.Offset(1 + m_CachedData.textureMap.Size(), unDescriptorInc);

	// Set render target
	CD3DX12_CPU_DESCRIPTOR_HANDLE d3dRTVCPUDescriptorHandle = RENDER->GetCurrentBackBufferHandle();
	pd3dCommandList->OMSetRenderTargets(1, &d3dRTVCPUDescriptorHandle, TRUE, nullptr);
}

void UIPass::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	constexpr uint32 rootParamUIData = std::to_underlying(ROOT_PARAMETER::UI_DATA);
	auto nLayers = UIBoard::g_unUILayers;

	const auto& pQuad = RENDER->GetQuadMesh();

	for (int i = nLayers - 1; i >= 0; --i) {
		const auto& textsInLayer = m_CachedData.textDatas[i];
		const auto& spritesInLayer = m_CachedData.spriteDatas[i];

		// Draw text first
		if (textsInLayer.size() != 0) {
			auto textSBuffer = RENDER->AllocSBuffer<UIRectData>(textsInLayer.size());
			textSBuffer.WriteData(textsInLayer);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamUIData, textSBuffer.GPUAddress);
			
			pd3dCommandList->SetPipelineState(m_pd3dTextPipelineState.Get());
			pQuad->Render(pd3dCommandList, textsInLayer.size());
		}

		// Draw sprite
		if (spritesInLayer.size() != 0) {
			auto spriteSBuffer = RENDER->AllocSBuffer<UIRectData>(spritesInLayer.size());
			spriteSBuffer.WriteData(spritesInLayer);
			pd3dCommandList->SetGraphicsRootShaderResourceView(rootParamUIData, spriteSBuffer.GPUAddress);

			pd3dCommandList->SetPipelineState(m_pd3dSpritePipelineState.Get());
			pQuad->Render(pd3dCommandList, spritesInLayer.size());
		}
	}
}

void UIPass::OnPostRender(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const RenderPassInput& input, OUT RenderPassOutput& output, OUT DescriptorHandle& outDescHandle)
{
	// Do nothing
}

void UIPass::CreatePipelineState()
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
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("UIRectVS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("UITextPS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dPipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
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

	HRESULT hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dTextPipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	{
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("UISpritePS");
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	hr = DEVICE->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dSpritePipelineState.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}
}
