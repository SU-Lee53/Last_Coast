#pragma once

enum class TEXTURE_TYPE : uint32 {
	ALBEDO = 0,
	NORMAL = 1,
	METALLIC = 2,
	EMISSION = 3,

	SPECULAR,
	DETAILED_ALBEDO,
	DETAILED_NORAML,
	RENDER_TARGET,

	DIFFUSE = ALBEDO,

	UNDEFINED = 0
};

class Texture {
	friend class TextureManager;

public:
	using ID = uint64;

public:
	void StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState);

	ShaderResource GetTexture() const { return m_pTexResource; }
	ComPtr<ID3D12Resource> GetResource() const { return m_pTexResource.pResource; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() { return m_d3dSRVHandle; }
	const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const { return m_d3dSRVDesc; }
	TEXTURE_TYPE GetType() const { return m_eType; }

private:
	bool CreateTextureFromFile(const std::wstring& wstrTexturePath);
	bool CreateTextureArrayFromFile(const std::wstring& wstrTexturePath);
	bool CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
	
	[[nodiscard]] 
	HRESULT LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);
	[[nodiscard]] 
	HRESULT LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

protected:
	ShaderResource m_pTexResource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dSRVHandle;
	D3D12_SHADER_RESOURCE_VIEW_DESC m_d3dSRVDesc;
	TEXTURE_TYPE m_eType;

	uint64 m_un64RuntimeSRVID;

private:
	inline static std::wstring g_wstrTextureBasePath = L"../Resources/Textures";
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

class RenderTargetTexture : public Texture {
	friend class TextureManager;
private:
	bool Initialize(
		uint32 unWidth, 
		uint32 unHeight, 
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN, 
		DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN);

	const D3D12_RENDER_TARGET_VIEW_DESC& GetRTVDesc() const { return m_d3dRTVDesc; }

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dRTVHandle;
	D3D12_RENDER_TARGET_VIEW_DESC m_d3dRTVDesc;

	uint64 m_un64RuntimeRTVID;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthStencilTexture

class DepthStencilTexture : public Texture {
private:
	void Initialize(
		UINT nWidth, 
		UINT nHeight,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiDSVFormat = DXGI_FORMAT_UNKNOWN);

	const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDSVDesc() const { return m_d3dRTVDesc; }

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dDSVHandle;
	D3D12_DEPTH_STENCIL_VIEW_DESC m_d3dRTVDesc;

	uint64 m_un64RuntimeDSVID;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// UnorderedAccessTexture

class UnorderedAccessTexture : public Texture {
private:
	void Initialize(
		UINT nWidth,
		UINT nHeight,
		DXGI_FORMAT dxgiSRVUAVFormat = DXGI_FORMAT_UNKNOWN);

	const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDSVDesc() const { return m_d3dRTVDesc; }

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dUAVHandle;
	D3D12_UNORDERED_ACCESS_VIEW_DESC m_d3dRTVDesc;

	uint64 m_un64RuntimeUAVID;
};
