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
	void StateTransition(
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList,
		D3D12_RESOURCE_STATES d3dAfterState);
	
	CD3DX12_RESOURCE_BARRIER GetResourceBarrier(
		D3D12_RESOURCE_STATES d3dAfterState,
		bool bChangeState = false);

	ComPtr<ID3D12Resource> GetResource() const { return m_pd3dResource; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() { return m_d3dSRVHandle; }
	const D3D12_SHADER_RESOURCE_VIEW_DESC& GetSRVDesc() const { return m_d3dSRVDesc; }

	bool HasTransparentPixel() const { return m_bHasTransparentPixel; }

private:
	bool CreateTextureFromFile(const std::wstring& wstrTexturePath, bool bCheckTransparent);
	bool CreateTextureArrayFromFile(const std::wstring& wstrTexturePath);
	bool CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
	
	[[nodiscard]] 
	HRESULT LoadFromDDSFile(
		ID3D12Resource** ppOutResource, 
		const std::wstring& wstrTexturePath, 
		std::unique_ptr<uint8_t[]>& ddsData, 
		std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

	[[nodiscard]] 
	HRESULT LoadFromWICFile(
		ID3D12Resource** ppOutResource, 
		const std::wstring& wstrTexturePath, 
		std::unique_ptr<uint8_t[]>& ddsData,
		std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

	bool AnalyzeTransparencyFromFile(const std::wstring& wstrTexturePath, float fThreshold);

protected:
	ComPtr<ID3D12Resource> m_pd3dResource = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dSRVHandle{};
	D3D12_SHADER_RESOURCE_VIEW_DESC m_d3dSRVDesc{};

	uint64 m_un64RuntimeSRVID;
	
	D3D12_RESOURCE_STATES m_d3dCurrentState;
	
	bool m_bHasTransparentPixel = false;

private:
	inline static std::wstring g_wstrTextureBasePath = L"../Resources/Textures";
	const static float g_fAlphaThreshold;

	static bool HasTransparentPixel_RGBA8(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight,
		float fThreshold
	);

	static bool HasTransparentPixel_RGBA8_SSE2(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight,
		uint8 threshold
	);

	static bool HasTransparentPixel_RGBA8_AVX2(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight,
		uint8 threshold
	);

	static bool HasTransparentPixel_RGBA8_Scalar(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight,
		uint8 threshold
	);

	static bool IsRGBA8Like(DXGI_FORMAT dxgiFormat);

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

class RenderTargetTexture : public Texture {
	friend class TextureManager;

public:
	const D3D12_RENDER_TARGET_VIEW_DESC& GetRTVDesc() const { return m_d3dRTVDesc; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() { return m_d3dRTVHandle; }

private:
	bool Initialize(
		uint32 unWidth, 
		uint32 unHeight, 
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN, 
		DXGI_FORMAT dxgiRTVFormat = DXGI_FORMAT_UNKNOWN);
	
	bool Initialize(ComPtr<ID3D12Resource> pd3dRTVResource);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dRTVHandle;
	D3D12_RENDER_TARGET_VIEW_DESC m_d3dRTVDesc;

	uint64 m_un64RuntimeRTVID;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// DepthStencilTexture

class DepthStencilTexture : public Texture {
	friend class TextureManager;

public:
	const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDSVDesc() const { return m_d3dDSVDesc; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() { return m_d3dDSVHandle; }

private:
	bool Initialize(
		UINT nWidth, 
		UINT nHeight,
		bool bMsaa4xEnable,
		DXGI_FORMAT dxgiSRVFormat = DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT dxgiDSVFormat = DXGI_FORMAT_UNKNOWN);

	bool Initialize(ComPtr<ID3D12Resource> pd3dDSVResource);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dDSVHandle;
	D3D12_DEPTH_STENCIL_VIEW_DESC m_d3dDSVDesc;

	uint64 m_un64RuntimeDSVID;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// UnorderedAccessTexture

class UnorderedAccessTexture : public Texture {
	friend class TextureManager;

public:
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetUAVDesc() const { return m_d3dUAVDesc; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetUAVHandle() { return m_d3dUAVHandle; }

private:
	bool Initialize(
		UINT nWidth,
		UINT nHeight,
		DXGI_FORMAT dxgiSRVUAVFormat = DXGI_FORMAT_UNKNOWN);

	bool InitializeArray(
		UINT nArraySize,
		UINT nWidth,
		UINT nHeight,
		DXGI_FORMAT dxgiSRVUAVFormat = DXGI_FORMAT_UNKNOWN);

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_d3dUAVHandle;
	D3D12_UNORDERED_ACCESS_VIEW_DESC m_d3dUAVDesc;

	uint64 m_un64RuntimeUAVID;
};
