#pragma once

enum class TEXTURE_TYPE : uint32 {
	TEXTURE_TYPE_ALBEDO,
	TEXTURE_TYPE_NORMAL,
	TEXTURE_TYPE_SPECULAR,
	TEXTURE_TYPE_METALLIC,
	TEXTURE_TYPE_EMISSION,
	TEXTURE_TYPE_DETAILED_ALBEDO,
	TEXTURE_TYPE_DETAILED_NORAML,
	TEXTURE_TYPE_RENDER_TARGET,

	TEXTURE_TYPE_DIFFUSE = TEXTURE_TYPE_ALBEDO,

	TEXTURE_TYPE_UNDEFINED = 0
};

class Texture {
	friend class TextureManager;

public:
	void StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState);

	ShaderResource GetTexture() const { return m_pTexResource; }
	ComPtr<ID3D12Resource> GetResource() const { return m_pTexResource.pResource; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() { return m_d3dSRVHandle; }
	const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const { return m_d3dSRVDesc; }
	TEXTURE_TYPE GetType() const { return m_eType; }

private:
	Texture();
	
	void CreateTextureFromFile(const std::wstring& wstrTexturePath);
	void CreateTextureArrayFromFile(const std::wstring& wstrTexturePath);
	void CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
		 
	void LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);
	void LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

protected:
	ShaderResource m_pTexResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dSRVHandle;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_d3dSRVDesc;
	TEXTURE_TYPE m_eType;

	uint64 m_un64RuntimeID;

private:
	inline static std::wstring g_wstrTextureBasePath = L"../Resources/Textures";
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

class RenderTargetTexture : public Texture {
	friend class TextureManager;
private:
	void Initialize(
		ComPtr<ID3D12Device> pd3dDevice, 
		uint32 unWidth, 
		uint32 unHeight, 
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN, 
		DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dRTVHandle;
	D3D12_RENDER_TARGET_VIEW_DESC m_d3dRTVDesc;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// UnorderedAccessTexture

class UnorderedAccessTexture : public Texture {
private:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dUAVHandle;
	D3D12_UNORDERED_ACCESS_VIEW_DESC m_d3dRTVDesc;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthStencilTexture

class DepthStencilTexture : public Texture {
private:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dDSVHandle;
	D3D12_DEPTH_STENCIL_VIEW_DESC m_d3dRTVDesc;


};
