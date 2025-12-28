#include "pch.h"
#include "Texture.h"

void Texture::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::wstring& wstrTexturePath)
{
}

void Texture::StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState)
{
	m_pTexResource.StateTransition(pd3dCommandList, d3dAfterState);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

void RenderTargetTexture::Initialize(ComPtr<ID3D12Device> pd3dDevice, UINT nWidth, UINT nHeight, DXGI_FORMAT dxgiFormat)
{
	D3D12_RESOURCE_DESC d3dRTTextureDesc;
	{
		d3dRTTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dRTTextureDesc.Alignment = 0;
		d3dRTTextureDesc.Width = nWidth;
		d3dRTTextureDesc.Height = nHeight;
		d3dRTTextureDesc.DepthOrArraySize = 1;
		d3dRTTextureDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dRTTextureDesc.SampleDesc.Count = 1;
		d3dRTTextureDesc.SampleDesc.Quality = 0;
		d3dRTTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dRTTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		d3dRTTextureDesc.MipLevels = 1;
	}

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = d3dRTTextureDesc.Format;
	clearValue.Color[0] = 0.f;
	clearValue.Color[1] = 0.f;
	clearValue.Color[2] = 0.f;
	clearValue.Color[3] = 1.f;

	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&d3dRTTextureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(m_pTexResource.pResource.GetAddressOf())
	);

}
