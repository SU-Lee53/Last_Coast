#include "pch.h"
#include "UIManager.h"
#include "Texture.h"

UIManager::UIManager(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;

	CreateRootSignature();
	CreatePipelineState();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1 + (MAX_SPRITE_PER_DRAW * 2);
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	HRESULT hr = m_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pd3dDescriptorHeap.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

}

void UIManager::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE d3dDescRanges[5];
	// Font 텍스쳐
	d3dDescRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	d3dDescRanges[0].NumDescriptors = 1;
	d3dDescRanges[0].BaseShaderRegister = 0;
	d3dDescRanges[0].RegisterSpace = 0;
	d3dDescRanges[0].OffsetInDescriptorsFromTableStart = 0;

	// RECT 정보
	d3dDescRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	d3dDescRanges[1].NumDescriptors = 1;
	d3dDescRanges[1].BaseShaderRegister = 0;
	d3dDescRanges[1].RegisterSpace = 0;
	d3dDescRanges[1].OffsetInDescriptorsFromTableStart = 0;
	
	// Text 정보
	d3dDescRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	d3dDescRanges[2].NumDescriptors = 1;
	d3dDescRanges[2].BaseShaderRegister = 1;
	d3dDescRanges[2].RegisterSpace = 0;
	d3dDescRanges[2].OffsetInDescriptorsFromTableStart = 0;

	// Texture
	d3dDescRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	d3dDescRanges[3].NumDescriptors = 1;
	d3dDescRanges[3].BaseShaderRegister = 1;
	d3dDescRanges[3].RegisterSpace = 0;
	d3dDescRanges[3].OffsetInDescriptorsFromTableStart = 0;

	// Billboard
	d3dDescRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	d3dDescRanges[4].NumDescriptors = 1;
	d3dDescRanges[4].BaseShaderRegister = 2;
	d3dDescRanges[4].RegisterSpace = 0;
	d3dDescRanges[4].OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER d3dRootParameters[5];
	// Font Texture
	d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	d3dRootParameters[0].DescriptorTable.pDescriptorRanges = &d3dDescRanges[0];
	d3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					
	// RECT
	d3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	d3dRootParameters[1].DescriptorTable.pDescriptorRanges = &d3dDescRanges[1];
	d3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// Text
	d3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	d3dRootParameters[2].DescriptorTable.pDescriptorRanges = &d3dDescRanges[2];
	d3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// Texture
	d3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	d3dRootParameters[3].DescriptorTable.pDescriptorRanges = &d3dDescRanges[3];
	d3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// Billboard data
	d3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	d3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	d3dRootParameters[4].DescriptorTable.pDescriptorRanges = &d3dDescRanges[4];
	d3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc;
	d3dSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dSamplerDesc.MipLODBias = 0;
	d3dSamplerDesc.MaxAnisotropy = 1;
	d3dSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dSamplerDesc.MinLOD = 0;
	d3dSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	d3dSamplerDesc.ShaderRegister = 0;
	d3dSamplerDesc.RegisterSpace = 0;
	d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc{};
	{
		d3dRootSignatureDesc.NumParameters = _countof(d3dRootParameters);
		d3dRootSignatureDesc.pParameters = d3dRootParameters;
		d3dRootSignatureDesc.NumStaticSamplers = 1;
		d3dRootSignatureDesc.pStaticSamplers = &d3dSamplerDesc;
		d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
	}

	ComPtr<ID3DBlob> pd3dSignatureBlob = nullptr;
	ComPtr<ID3DBlob> pd3dErrorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pd3dSignatureBlob.GetAddressOf(), pd3dErrorBlob.GetAddressOf());
	if (pd3dErrorBlob) {
		char* pErrorString = (char*)pd3dErrorBlob->GetBufferPointer();
		HWND hWnd = ::GetActiveWindow();
		MessageBoxA(hWnd, pErrorString, NULL, 0);
		OutputDebugStringA(pErrorString);
		__debugbreak();
	}

	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pd3dUIRootSignature.GetAddressOf()));

}

void UIManager::CreatePipelineState()
{
	ComPtr<ID3DBlob> m_pd3dVertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dGeometryShaderBlob = nullptr;
	ComPtr<ID3DBlob> m_pd3dPixelShaderBlob = nullptr;

	// m_pd3dUIPipelineState[0] : Texture UI
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc{};
	{
		d3dPipelineDesc.pRootSignature = m_pd3dUIRootSignature.Get();
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TextureSpriteVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("TextureSpriteGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TextureSpritePS");
		d3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = true;
		d3dPipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		d3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		d3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		d3dPipelineDesc.DepthStencilState.DepthEnable = false;
		d3dPipelineDesc.DepthStencilState.StencilEnable = false;
		d3dPipelineDesc.InputLayout.NumElements = 0;
		d3dPipelineDesc.InputLayout.pInputElementDescs = nullptr;
		d3dPipelineDesc.SampleMask = UINT_MAX;
		d3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;	// GS가 점을 받음
		d3dPipelineDesc.NumRenderTargets = 1;
		d3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		d3dPipelineDesc.SampleDesc.Count = 1;
		d3dPipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	}

	HRESULT hr = m_pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dUIPipelineStates[0].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// m_pd3dUIPipelineState[1] : Text UI
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("TextSpriteVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("TextSpriteGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("TextSpritePS");
	}

	hr = m_pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dUIPipelineStates[1].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// m_pd3dUIPipelineState[2] : Billboard UI
	{
		d3dPipelineDesc.VS = SHADER->GetShaderByteCode("BillboardSpriteVS");
		d3dPipelineDesc.GS = SHADER->GetShaderByteCode("BillboardSpriteGS");
		d3dPipelineDesc.PS = SHADER->GetShaderByteCode("BillboardSpritePS");
		d3dPipelineDesc.DepthStencilState.DepthEnable = true;
	}

	hr = m_pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineDesc, IID_PPV_ARGS(m_pd3dUIPipelineStates[2].GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
	}

	// 11.10
	// TODO : Root Signature, Pipeline State 재설정
}

void UIManager::Add(std::shared_ptr<Sprite> pSprite, UINT nSpriteType, UINT nLayerIndex)
{
	if (nSpriteType != SPRITE_TYPE_BILLBOARD) {
		m_UILayer.Add(pSprite, nSpriteType, nLayerIndex);
	}
	else {
		m_pBillboardsInWorld.push_back(pSprite);
	}
}

void UIManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dUIRootSignature.Get());
	pd3dCommandList->SetDescriptorHeaps(1, m_pd3dDescriptorHeap.GetAddressOf());
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	DescriptorHandle descHandle{ 
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart()), 
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pd3dDescriptorHeap->GetGPUDescriptorHandleForHeapStart()) 
	};

	std::shared_ptr<Texture> pFontTex = TEXTURE->Get("font");

	m_pd3dDevice->CopyDescriptorsSimple(1, descHandle.cpuHandle, pFontTex->GetHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.cpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	pd3dCommandList->SetGraphicsRootDescriptorTable(0, descHandle.gpuHandle);
	descHandle.gpuHandle.Offset(1, D3DCore::g_nCBVSRVDescriptorIncrementSize);

	// 0번부터 그림
	// 즉, 0번이 제일 뒤, MAX_UI_LAYER_COUNT - 1번이 가장 앞
	auto Layers = m_UILayer.GetLayers();

	for (const auto& layer : Layers) {
		for (int i = 0; i < SPRITE_TYPE_COUNT; ++i) {
			pd3dCommandList->SetPipelineState(m_pd3dUIPipelineStates[i].Get());

			for (const auto& sprite : layer[i]) {
				sprite->Render(m_pd3dDevice, pd3dCommandList, descHandle);
			}
		}
	}

	for (const auto& pBillboard : m_pBillboardsInWorld) {
		pBillboard->Render(m_pd3dDevice, pd3dCommandList, descHandle);
	}
}

void UIManager::Clear()
{
	m_UILayer.Clear();
	m_pBillboardsInWorld.clear();
}

void UIManager::CheckButtonClicked(POINT ptClickedPos) 
{

}
