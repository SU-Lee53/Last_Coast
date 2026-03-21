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
	enum class ALPHA_MODE : uint32 {
		Opaque = 0,
		Masked = 1,
		Transparent = 2
	};

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

	//bool HasTransparentPixel() const { return m_bHasTransparentPixel; }
	Texture::ALPHA_MODE GetAlphaMode() const { return m_eAlphaMode; }

	virtual void ShowDebugInfo() const;

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

	Texture::ID m_un64RuntimeSRVID;
	
	D3D12_RESOURCE_STATES m_d3dCurrentState;
	
	//bool m_bHasTransparentPixel = false;
	Texture::ALPHA_MODE m_eAlphaMode = ALPHA_MODE::Opaque;

private:
	inline static std::wstring g_wstrTextureBasePath = L"../Resources/Textures";
	const static float g_fAlphaThreshold;

	struct AlphaAnalysisResult
	{
		bool bHasAnyNonOpaqueAlpha = false;
		bool bHasZeroAlpha = false;
		bool bHasMidAlpha = false;

		size_t unZeroCount = 0;
		size_t unOneCount = 0;
		size_t unMidCount = 0;
		size_t unTotalCount = 0;

		float fZeroRatio = 0.f;
		float fOneRatio = 0.f;
		float fMidRatio = 0.f;

		Texture::ALPHA_MODE eSuggestedMode = Texture::ALPHA_MODE::Opaque;
	};

	static Texture::AlphaAnalysisResult AnalyzeAlphaRGBA8_Scalar(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight
	);

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

	static bool HasTransparentPixel_RGBA8_Scalar(
		const uint8* pixels,
		size_t unRowPitch,
		size_t unWidth,
		size_t unHeight,
		uint8 threshold
	);

	static bool IsRGBA8Like(DXGI_FORMAT dxgiFormat);

	static std::string AlphaModeToString(ALPHA_MODE m) {
		switch (m)
		{
		case Texture::ALPHA_MODE::Opaque: return "ALPHA_MODE::Opaque";
		case Texture::ALPHA_MODE::Masked: return "ALPHA_MODE::Masked";
		case Texture::ALPHA_MODE::Transparent: return "ALPHA_MODE::Transparent";
		}

		return {};
	}

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTargetTexture

class RenderTargetTexture : public Texture {
	friend class TextureManager;

public:
	const D3D12_RENDER_TARGET_VIEW_DESC& GetRTVDesc() const { return m_d3dRTVDesc; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() { return m_d3dRTVHandle; }
	virtual void ShowDebugInfo() const;

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
	virtual void ShowDebugInfo() const;

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
	virtual void ShowDebugInfo() const;

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
