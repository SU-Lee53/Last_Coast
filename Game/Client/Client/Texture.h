#pragma once

enum TEXTURE_TYPE : UINT8 {
	TEXTURE_TYPE_ALBEDO = 0x01,
	TEXTURE_TYPE_NORMAL = 0x02,
	TEXTURE_TYPE_SPECULAR = 0x04,
	TEXTURE_TYPE_METALLIC = 0x08,
	TEXTURE_TYPE_EMISSION = 0x10,
	TEXTURE_TYPE_DETAILED_ALBEDO = 0x20,
	TEXTURE_TYPE_DETAILED_NORAML = 0x40,
	TEXTURE_TYPE_RENDER_TARGET,

	TEXTURE_TYPE_DIFFUSE = TEXTURE_TYPE_ALBEDO,

	TEXTURE_TYPE_UNDEFINED = 0
};

class Texture {
	friend class TextureManager;

private:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, const std::wstring& wstrTexturePath);

public:
	void StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState);

public:
	ShaderResource GetTexture() const { return m_pTexResource; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() { return m_d3dSRVHandle; }
	const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const { return m_d3dSRVDesc; }
	UINT GetType() const { return m_eType; }

protected:
	ShaderResource m_pTexResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dSRVHandle;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_d3dSRVDesc;
	UINT m_eType;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

class RenderTargetTexture : public Texture {
	friend class TextureManager;
private:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, UINT nWidth, UINT nHeight, DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dRTVHandle;

};

class UnorderedAccessTexture : public Texture {
private:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight);

private:

};