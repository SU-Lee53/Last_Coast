#include "pch.h"
#include "Texture.h"
#include <xmmintrin.h>
#include <emmintrin.h>

const float Texture::g_fAlphaThreshold = 0.98f;

bool Texture::CreateTextureFromFile(const std::wstring& wstrTextureName, bool bCheckTransparent)
{
	namespace fs = std::filesystem;

	std::wstring wstrTexturePath;
	if (fs::path(wstrTextureName).has_extension()) {
		wstrTexturePath = wstrTextureName;
	}
	else {
		wstrTexturePath = std::format(L"{}/{}.dds", g_wstrTextureBasePath, wstrTextureName);
	}

	fs::path texPath{ wstrTexturePath };
	if (!fs::exists(texPath)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", texPath.string()).c_str());
		return false;
	}

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subResources;
	auto isDDS = [](const fs::path& path) -> bool {return path.extension().string() == ".dds" || path.extension().string() == ".DDS"; };
	HRESULT hr;
	hr = isDDS(texPath) ? LoadFromDDSFile(m_pd3dResource.GetAddressOf(), wstrTexturePath, ddsData, subResources) 
		                : LoadFromWICFile(m_pd3dResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture load failed", texPath.string()).c_str());
		return false;
	}

	if (bCheckTransparent) {
		bool bResult = AnalyzeTransparencyFromFile(wstrTexturePath, g_fAlphaThreshold);
		if (!bResult)
		{
			OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Transparency analysis failed", texPath.string()).c_str());
			//m_bHasTransparentPixel = false;
			m_eAlphaMode = ALPHA_MODE::Opaque;
		}
	}

	UINT nSubResources = (UINT)subResources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(m_pd3dResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;

	TEXTURE->UpdateResources(m_pd3dResource, m_d3dCurrentState, subResources, nBytes);

	return true;
}

bool Texture::CreateTextureArrayFromFile(const std::wstring& wstrTexturePath)
{
	namespace fs = std::filesystem;

	fs::path texPath{ wstrTexturePath };
	if (!fs::exists(texPath)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", texPath.string()).c_str());
		return false;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subResources;
	auto isDDS = [](const fs::path& path) -> bool {return path.extension().string() == ".dds" || path.extension().string() == ".DDS"; };
	HRESULT hr;
	hr = isDDS(texPath) ? LoadFromDDSFile(m_pd3dResource.GetAddressOf(), wstrTexturePath, ddsData, subResources)
		: LoadFromWICFile(m_pd3dResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture load failed", texPath.string()).c_str());
		return false;
	}

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = m_pd3dResource->GetDesc();
	UINT nSubResources = (UINT)subResources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(m_pd3dResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;

	TEXTURE->UpdateResources(m_pd3dResource, m_d3dCurrentState, subResources, nBytes);

	return true;
}

bool Texture::CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat)
{
	namespace fs = std::filesystem;

	//std::wstring wstrTexturePath;
	std::ifstream in{ wstrTexturePath, std::ios::binary };
	if (!in) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", fs::path(wstrTexturePath).string()).c_str());
		return false;
	}

	// 파일 읽기
	in.seekg(0, std::ios::end);
	int32 nSize = in.tellg();
	in.seekg(0, std::ios::beg);

	std::vector<uint8> rawData;
	rawData.resize(nSize);
	in.read((char*)rawData.data(), nSize);

	// 리소스 포인터 생성
	D3D12_RESOURCE_DESC resourceDesc = {};
	{
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = unWidth;
		resourceDesc.Height = unHeight;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = dxgiFormat;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();
	CD3DX12_HEAP_PROPERTIES d3dHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	// UploadBuffer 생성
	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pd3dResource->GetDesc();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pd3dResource.Get(), 0, 1);

	ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr;
	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pd3dUploadBuffer.GetAddressOf())
	);

	// UploadBuffer 에 바로 Raw data 복사
	uint8* pMappedPtr = nullptr;
	CD3DX12_RANGE d3dReadRange(0, 0);
	pd3dUploadBuffer->Map(0, &d3dReadRange, reinterpret_cast<void**>(&pMappedPtr));
	::memcpy(pMappedPtr, rawData.data(), rawData.size());
	pd3dUploadBuffer->Unmap(0, nullptr);

	std::vector<D3D12_SUBRESOURCE_DATA> subResources(1);
	subResources[0].pData = pMappedPtr;
	subResources[0].RowPitch = unWidth * sizeof(uint32); // R8G8B8A8
	subResources[0].SlicePitch = subResources[0].RowPitch * unHeight;

	TEXTURE->UpdateResources(m_pd3dResource, m_d3dCurrentState, subResources, nBytes, pd3dUploadBuffer);

	return true;
}

bool Texture::CreateTextureFromRawData(const std::wstring& wstrTexturePath, const std::vector<Vector4> data, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat)
{
	// 리소스 포인터 생성
	D3D12_RESOURCE_DESC resourceDesc = {};
	{
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = unWidth;
		resourceDesc.Height = unHeight;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = dxgiFormat;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	CD3DX12_HEAP_PROPERTIES d3dHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	// UploadBuffer 생성
	D3D12_RESOURCE_DESC d3dTextureResourceDesc = m_pd3dResource->GetDesc();
	UINT64 nBytes = GetRequiredIntermediateSize(m_pd3dResource.Get(), 0, 1);

	ComPtr<ID3D12Resource> pd3dUploadBuffer = nullptr;
	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pd3dUploadBuffer.GetAddressOf())
	);

	// UploadBuffer 에 바로 Raw data 복사
	uint8* pMappedPtr = nullptr;
	CD3DX12_RANGE d3dReadRange(0, 0);
	pd3dUploadBuffer->Map(0, &d3dReadRange, reinterpret_cast<void**>(&pMappedPtr));
	::memcpy(pMappedPtr, data.data(), data.size() * sizeof(decltype(data)::value_type));
	pd3dUploadBuffer->Unmap(0, nullptr);

	std::vector<D3D12_SUBRESOURCE_DATA> subResources(1);
	subResources[0].pData = pMappedPtr;
	subResources[0].RowPitch = unWidth * sizeof(uint32); // R8G8B8A8
	subResources[0].SlicePitch = subResources[0].RowPitch * unHeight;

	TEXTURE->UpdateResources(m_pd3dResource, m_d3dCurrentState, subResources, nBytes, pd3dUploadBuffer);

	return true;
}

HRESULT Texture::LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
{
	HRESULT hr;
	DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool bIsCubeMap = false;

	hr = ::LoadDDSTextureFromFileEx(
		DEVICE.Get(),
		wstrTexturePath.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		DDS_LOADER_DEFAULT,
		ppOutResource,
		ddsData,
		subResources,
		&ddsAlphaMode,
		&bIsCubeMap
	);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {}", __FILE__, __LINE__, "Failed To Load DDS File").c_str());
	}

	return hr;
}
	
HRESULT Texture::LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
{
	HRESULT hr;

	subResources.resize(1);

	hr = ::LoadWICTextureFromFileEx(
		DEVICE.Get(),
		wstrTexturePath.c_str(),
		0,
		D3D12_RESOURCE_FLAG_NONE,
		WIC_LOADER_IGNORE_SRGB,
		ppOutResource,
		ddsData,
		subResources[0]
	);

	if (FAILED(hr)) {
		OutputDebugStringA(std::format("{} - {} : {}", __FILE__, __LINE__, "Failed To Load WIC File").c_str());
	}

	return hr;
}

bool Texture::AnalyzeTransparencyFromFile(const std::wstring& wstrTexturePath, float fThreshold)
{
	namespace fs = std::filesystem;
	
	TexMetadata metaData{};
	ScratchImage image;
	HRESULT hr{};

	auto isDDS = [](const fs::path& path) -> bool {return path.extension().string() == ".dds" || path.extension().string() == ".DDS"; };
	const fs::path p(wstrTexturePath);

	// Read DDS/WIC from file again using DirectXTex
	if (isDDS(p)) {
		hr = ::LoadFromDDSFile(
			wstrTexturePath.c_str(),
			DDS_FLAGS_NONE,
			&metaData,
			image
		);
	}
	else {
		hr = ::LoadFromWICFile(
			wstrTexturePath.c_str(),
			WIC_FLAGS_IGNORE_SRGB,
			&metaData,
			image
		);
	}

	if (FAILED(hr)) {
		return false;
	}

	// Check format first and end if no possibility of alpha channel
	if (!::HasAlpha(metaData.format)) {
		//m_bHasTransparentPixel = false;
		m_eAlphaMode = ALPHA_MODE::Opaque;
		return true;
	}

	ScratchImage workingImage;
	const Image* pScanImage = nullptr;

	// Decompress if texture is BC compressed format
	if (::IsCompressed(metaData.format)) {
		hr = ::Decompress(
			image.GetImages(),
			image.GetImageCount(),
			metaData,
			DXGI_FORMAT_UNKNOWN,
			workingImage
		);

		if (FAILED(hr)) {
			return false;
		}

		pScanImage = workingImage.GetImage(0, 0, 0);	// miplevel = 0
	}
	else {
		pScanImage = image.GetImage(0, 0, 0);
	}

	if (!pScanImage) {
		return false;
	}

	ScratchImage convertedImage;
	const Image* pFinalImage = pScanImage;

	// Convert to RGBA8 if format is not 8-bit
	if (!Texture::IsRGBA8Like(pScanImage->format)) {
		hr = ::Convert(
			*pScanImage,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			TEX_FILTER_DEFAULT,
			TEX_THRESHOLD_DEFAULT,
			convertedImage
		);

		if (FAILED(hr)) {
			return false;
		}

		pFinalImage = convertedImage.GetImage(0, 0, 0);
		if (!pFinalImage) {
			return false;
		}
	}

	// Check alpha byte using SIMD
	//m_bHasTransparentPixel = Texture::HasTransparentPixel_RGBA8(
	//	pFinalImage->pixels,
	//	pFinalImage->rowPitch,
	//	pFinalImage->width,
	//	pFinalImage->height,
	//	fThreshold
	//);

	AlphaAnalysisResult result = Texture::AnalyzeAlphaRGBA8_Scalar(
		pFinalImage->pixels,
		pFinalImage->rowPitch,
		pFinalImage->width,
		pFinalImage->height
	);

	m_eAlphaMode = result.eSuggestedMode;

	return true;
}

Texture::AlphaAnalysisResult Texture::AnalyzeAlphaRGBA8_Scalar(const uint8* pixels, size_t unRowPitch, size_t unWidth, size_t unHeight)
{
	AlphaAnalysisResult result{};

	if (!pixels || unWidth == 0 || unHeight == 0) {
		return result;
	}

	constexpr uint8 zeroMax = 8;
	constexpr uint8 oneMin = 247;

	for (size_t y = 0; y < unHeight; ++y) {
		const uint8* pRow = pixels + y * unRowPitch;
		
		for (size_t x = 0; x < unWidth; ++x) {
			const uint8 alpha = pRow[x * 4 + 3];
			++result.unTotalCount;

			if (alpha <= zeroMax) {
				++result.unZeroCount;
				result.bHasZeroAlpha = true;
				result.bHasAnyNonOpaqueAlpha = true;
			}
			else if (alpha > oneMin) {
				++result.unOneCount;
			}
			else {
				++result.unMidCount;
				result.bHasMidAlpha = true;
				result.bHasAnyNonOpaqueAlpha = true;
			}
		}
	}

	if (result.unTotalCount > 0) {
		const float fInvTotal = 1.0f / static_cast<float>(result.unTotalCount);
		result.fZeroRatio = static_cast<float>(result.unZeroCount) * fInvTotal;
		result.fOneRatio = static_cast<float>(result.unOneCount) * fInvTotal;
		result.fMidRatio = static_cast<float>(result.unMidCount) * fInvTotal;
	}

	if (!result.bHasAnyNonOpaqueAlpha) {
		result.eSuggestedMode = ALPHA_MODE::Opaque;
	}
	else if (result.fZeroRatio > 0.5f) {
		result.eSuggestedMode = ALPHA_MODE::Masked;
	}
	else {
		result.eSuggestedMode = ALPHA_MODE::Transparent;
	}

	return result;
}

bool Texture::HasTransparentPixel_RGBA8(const uint8* pixels, size_t unRowPitch, size_t unWidth, size_t unHeight, float fThreshold)
{
	float t = std::clamp(fThreshold, 0.f, 1.f);
	const uint8 threshold = static_cast<uint8>(std::lround(t * 255.0f));

	bool bResult = false;
	bResult = D3DCore::g_bSupportSSE2 ? HasTransparentPixel_RGBA8_SSE2(pixels, unRowPitch, unWidth, unHeight, threshold)
		                              : HasTransparentPixel_RGBA8_Scalar(pixels, unRowPitch, unWidth, unHeight, threshold);

	return bResult;
}

bool Texture::HasTransparentPixel_RGBA8_SSE2(const uint8* pixels, size_t unRowPitch, size_t unWidth, size_t unHeight, uint8 threshold)
{
	if (!pixels || unWidth == 0 || unHeight == 0) {
		return false;
	}

	const __m128i thresholdVector = _mm_set1_epi32(static_cast<int>(threshold));

	for (size_t y = 0; y < unHeight; ++y) {
		const uint8_t* pRow = pixels + (y * unRowPitch);
		size_t x = 0;
		
		// Processing 4 pixels at a time
		for (; x + 4 <= unWidth; x += 4) {
			const __m128i px = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pRow + (x * 4)));

			// 각 픽셀의 alpha 바이트를 32비트 lane 으로 추출
			// Extract each pixel's alpha byte to 32 bit lane
			const uint32_t a0 = static_cast<uint32_t>(_mm_extract_epi16(px, 1) >> 8);
			const uint32_t a1 = static_cast<uint32_t>(_mm_extract_epi16(px, 3) >> 8);
			const uint32_t a2 = static_cast<uint32_t>(_mm_extract_epi16(px, 5) >> 8);
			const uint32_t a3 = static_cast<uint32_t>(_mm_extract_epi16(px, 7) >> 8);

			const __m128i alphaVector = _mm_setr_epi32(a0, a1, a2, a3);

			// threshold > alpha
			const __m128i cmp = _mm_cmpgt_epi32(thresholdVector, alphaVector);
			if (_mm_movemask_ps(_mm_castsi128_ps(cmp)) != 0) {
				return true;
			}
		}

		// remainder
		for (; x < unWidth; ++x) {
			const uint8 alpha = pRow[(x * 4) + 3];
			if (alpha < threshold) {
				return true;
			}
		}
	}

	return false;
}

bool Texture::HasTransparentPixel_RGBA8_Scalar(const uint8* pixels, size_t unRowPitch, size_t unWidth, size_t unHeight, uint8 threshold)
{
	if (!pixels || unWidth == 0 || unHeight == 0) {
		return false;
	}

	for (size_t y = 0; y < unHeight; ++y) {
		const uint8_t* row = pixels + y * unRowPitch;
		for (size_t x = 0; x < unWidth; ++x) {
			if (row[x * 4 + 3] < threshold)
				return true;
		}
	}

	return false;
}

bool Texture::IsRGBA8Like(DXGI_FORMAT dxgiFormat)
{
	switch (dxgiFormat) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return true;

	default:
		return false;
	}
}

void Texture::StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState)
{
	if (m_d3dCurrentState == d3dAfterState) {
		//__debugbreak();
		return;
	}

	pd3dCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_pd3dResource.Get(),
			m_d3dCurrentState,
			d3dAfterState,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAG_NONE)
	);

	m_d3dCurrentState = d3dAfterState;
}

CD3DX12_RESOURCE_BARRIER Texture::GetResourceBarrier(D3D12_RESOURCE_STATES d3dAfterState, bool bChangeState)
{
	if (m_d3dCurrentState == d3dAfterState) {
		//__debugbreak();
		return CD3DX12_RESOURCE_BARRIER{};
	}

	CD3DX12_RESOURCE_BARRIER d3dResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pd3dResource.Get(),
		m_d3dCurrentState,
		d3dAfterState,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_BARRIER_FLAG_NONE);

	m_d3dCurrentState = (bChangeState) ? d3dAfterState : m_d3dCurrentState;

	return d3dResourceBarrier;
}

bool RenderTargetTexture::Initialize(uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat, D3D12_RESOURCE_STATES d3dInitialState, float* pfClearValue)
{
	D3D12_RESOURCE_DESC d3dRTTextureDesc;
	{
		d3dRTTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dRTTextureDesc.Alignment = 0;
		d3dRTTextureDesc.Width = unWidth;
		d3dRTTextureDesc.Height = unHeight;
		d3dRTTextureDesc.DepthOrArraySize = 1;
		d3dRTTextureDesc.Format = dxgiRTVFormat;
		d3dRTTextureDesc.SampleDesc.Count = 1;
		d3dRTTextureDesc.SampleDesc.Quality = 0;
		d3dRTTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dRTTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		d3dRTTextureDesc.MipLevels = 1;
	}

	D3D12_CLEAR_VALUE clearValue = {};
	if (pfClearValue) {
		clearValue.Format = d3dRTTextureDesc.Format;
		::memcpy(clearValue.Color, pfClearValue, 4 * sizeof(float));
	}
	else {
		float pfClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
		clearValue.Format = d3dRTTextureDesc.Format;
		::memcpy(clearValue.Color, pfClearColor, 4 * sizeof(float));
	}
	
	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dRTTextureDesc,
		d3dInitialState,
		&clearValue,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);


	m_d3dCurrentState = d3dInitialState;
	return true;
}

bool RenderTargetTexture::Initialize(ComPtr<ID3D12Resource> pd3dRTVResource)
{
	m_pd3dResource = pd3dRTVResource;
	m_d3dCurrentState = D3D12_RESOURCE_STATE_PRESENT;

	return true;
}

bool DepthStencilTexture::Initialize(UINT nWidth, UINT nHeight, bool bMsaa4xEnable, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiDSVFormat)
{
	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dResourceDesc.Alignment = 0;
		d3dResourceDesc.Width = nWidth;
		d3dResourceDesc.Height = nHeight;
		d3dResourceDesc.DepthOrArraySize = 1;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = (dxgiDSVFormat == DXGI_FORMAT_D32_FLOAT) ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R24G8_TYPELESS;
		d3dResourceDesc.SampleDesc.Count = (bMsaa4xEnable) ? 4 : 1;
		d3dResourceDesc.SampleDesc.Quality = (bMsaa4xEnable) ? (bMsaa4xEnable - 1) : 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = dxgiDSVFormat;
	d3dClearValue.DepthStencil.Depth = 1.f;
	d3dClearValue.DepthStencil.Stencil = 0;

	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&d3dClearValue,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	m_d3dCurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	return true;
}

bool DepthStencilTexture::Initialize(ComPtr<ID3D12Resource> pd3dDSVResource)
{
	m_pd3dResource = pd3dDSVResource;
	m_d3dCurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	return true;
}

bool UnorderedAccessTexture::Initialize(UINT nWidth, UINT nHeight, DXGI_FORMAT dxgiSRVUAVFormat)
{
	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dResourceDesc.Width = nWidth;
		d3dResourceDesc.Height = nHeight;
		d3dResourceDesc.DepthOrArraySize = 1;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = dxgiSRVUAVFormat;
		d3dResourceDesc.SampleDesc.Count = 1;
		d3dResourceDesc.SampleDesc.Quality = 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_d3dCurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	return true;
}

bool UnorderedAccessTexture::Initialize3D(UINT nWidth, UINT nHeight, UINT nDepth, DXGI_FORMAT dxgiSRVUAVFormat)
{
	// TODO : 구현
	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		d3dResourceDesc.Width = nWidth;
		d3dResourceDesc.Height = nHeight;
		d3dResourceDesc.DepthOrArraySize = nDepth;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = dxgiSRVUAVFormat;
		d3dResourceDesc.SampleDesc.Count = 1;
		d3dResourceDesc.SampleDesc.Quality = 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_d3dCurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	return true;
}

bool UnorderedAccessTexture::InitializeArray(UINT nArraySize, UINT nWidth, UINT nHeight, DXGI_FORMAT dxgiSRVUAVFormat)
{
	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dResourceDesc.Width = nWidth;
		d3dResourceDesc.Height = nHeight;
		d3dResourceDesc.DepthOrArraySize = nArraySize;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = dxgiSRVUAVFormat;
		d3dResourceDesc.SampleDesc.Count = 1;
		d3dResourceDesc.SampleDesc.Quality = 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_d3dCurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	return true;
}

bool RWRenderTargetTexture::Initialize(UINT nWidth, UINT nHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat, DXGI_FORMAT dxgiUAVFormat)
{
	if (nWidth == 0 || nHeight == 0) {
		__debugbreak();
		return false;
	}

	if (dxgiRTVFormat == DXGI_FORMAT_UNKNOWN) {
		dxgiRTVFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	}

	if (dxgiSRVFormat == DXGI_FORMAT_UNKNOWN) {
		dxgiSRVFormat = dxgiRTVFormat;
	}

	if (dxgiUAVFormat == DXGI_FORMAT_UNKNOWN) {
		dxgiUAVFormat = dxgiRTVFormat;
	}

	D3D12_RESOURCE_DESC d3dResourceDesc{};
	{
		d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dResourceDesc.Alignment = 0;
		d3dResourceDesc.Width = nWidth;
		d3dResourceDesc.Height = nHeight;
		d3dResourceDesc.DepthOrArraySize = 1;
		d3dResourceDesc.MipLevels = 1;
		d3dResourceDesc.Format = dxgiRTVFormat;
		d3dResourceDesc.SampleDesc.Count = 1;
		d3dResourceDesc.SampleDesc.Quality = 0;
		d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	float pfClearColor[4] = { 0.f, 0.f, 0.f, 1.f };

	D3D12_CLEAR_VALUE d3dClearValue{};
	d3dClearValue.Format = dxgiRTVFormat;
	::memcpy(d3dClearValue.Color, pfClearColor, sizeof(pfClearColor));

	HRESULT hr = DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		&d3dClearValue,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_d3dCurrentState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ImGui Debug

void Texture::ShowDebugInfo() const
{
	ImGui::Indent(20.f);
	ImGui::Text("Resource State : %s", ::ResourceStateToString(m_d3dCurrentState).c_str());
	ImGui::Text("Alpha Mode : %s", AlphaModeToString(m_eAlphaMode).c_str());

	ImGui::Text("Runtime SRVID: %d", m_un64RuntimeSRVID);
	
	ImGui::SeparatorText("SRVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dSRVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::SRVViewDimensionsToString(m_d3dSRVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dSRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipLevels : %d", m_d3dSRVDesc.Texture2D.MipLevels);
			ImGui::Text("Texture2D.MostDetailedMip : %d", m_d3dSRVDesc.Texture2D.MostDetailedMip);
			ImGui::Text("Texture2D.PlaneSlice : %d", m_d3dSRVDesc.Texture2D.PlaneSlice);
			ImGui::Text("Texture2D.ResourceMinLODClamp : %d", m_d3dSRVDesc.Texture2D.ResourceMinLODClamp);
		}
		else if (m_d3dSRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) {
			ImGui::Text("Texture2DArray.MipLevels : %d", m_d3dSRVDesc.Texture2DArray.MipLevels);
			ImGui::Text("Texture2DArray.MostDetailedMip : %d", m_d3dSRVDesc.Texture2DArray.MostDetailedMip);
			ImGui::Text("Texture2DArray.PlaneSlice : %d", m_d3dSRVDesc.Texture2DArray.PlaneSlice);
			ImGui::Text("Texture2DArray.ArraySize : %d", m_d3dSRVDesc.Texture2DArray.ArraySize);
			ImGui::Text("Texture2DArray.FirstArraySlice : %d", m_d3dSRVDesc.Texture2DArray.FirstArraySlice);
			ImGui::Text("Texture2DArray.ResourceMinLODClamp : %d", m_d3dSRVDesc.Texture2DArray.ResourceMinLODClamp);
		}
		else if (m_d3dSRVDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBE) {
			ImGui::Text("TextureCube.MipLevels : %d", m_d3dSRVDesc.TextureCube.MipLevels);
			ImGui::Text("TextureCube.MostDetailedMip : %d", m_d3dSRVDesc.TextureCube.MostDetailedMip);
			ImGui::Text("TextureCube.ResourceMinLODClamp : %d", m_d3dSRVDesc.TextureCube.ResourceMinLODClamp);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Unindent(20.f);
}

void RenderTargetTexture::ShowDebugInfo() const
{
	Texture::ShowDebugInfo();

	ImGui::Indent(20.f);
	ImGui::Text("Runtime RTVID: %d", m_un64RuntimeRTVID);

	ImGui::SeparatorText("RTVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dRTVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::RTVViewDimensionsToString(m_d3dRTVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dRTVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipSlice : %d", m_d3dRTVDesc.Texture2D.MipSlice);
			ImGui::Text("Texture2D.PlaneSlice : %d", m_d3dRTVDesc.Texture2D.PlaneSlice);
		}
		else if (m_d3dRTVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) {
			ImGui::Text("Texture2DArray.MipSlice : %d", m_d3dRTVDesc.Texture2DArray.MipSlice);
			ImGui::Text("Texture2DArray.PlaneSlice : %d", m_d3dRTVDesc.Texture2DArray.PlaneSlice);
			ImGui::Text("Texture2DArray.ArraySize : %d", m_d3dRTVDesc.Texture2DArray.ArraySize);
			ImGui::Text("Texture2DArray.FirstArraySlice : %d", m_d3dRTVDesc.Texture2DArray.FirstArraySlice);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Unindent(20.f);
}

void DepthStencilTexture::ShowDebugInfo() const
{
	Texture::ShowDebugInfo();

	ImGui::Indent(20.f);
	ImGui::Text("Runtime DSVID: %d", m_un64RuntimeDSVID);

	ImGui::SeparatorText("DSVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dDSVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::DSVViewDimensionsToString(m_d3dDSVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dDSVDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipSlice : %d", m_d3dDSVDesc.Texture2D.MipSlice);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Unindent(20.f);
}

void UnorderedAccessTexture::ShowDebugInfo() const
{
	Texture::ShowDebugInfo();

	ImGui::Indent(20.f);
	ImGui::Text("Runtime UAVID: %d", m_un64RuntimeUAVID);

	ImGui::SeparatorText("UAVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dUAVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::UAVViewDimensionsToString(m_d3dUAVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dUAVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipSlice : %d", m_d3dUAVDesc.Texture2D.MipSlice);
			ImGui::Text("Texture2D.PlaneSlice : %d", m_d3dUAVDesc.Texture2D.PlaneSlice);
		}
		else if (m_d3dUAVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) {
			ImGui::Text("Texture2DArray.MipSlice : %d", m_d3dUAVDesc.Texture2DArray.MipSlice);
			ImGui::Text("Texture2DArray.PlaneSlice : %d", m_d3dUAVDesc.Texture2DArray.PlaneSlice);
			ImGui::Text("Texture2DArray.ArraySize : %d", m_d3dUAVDesc.Texture2DArray.ArraySize);
			ImGui::Text("Texture2DArray.FirstArraySlice : %d", m_d3dUAVDesc.Texture2DArray.FirstArraySlice);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Unindent(20.f);
}

void RWRenderTargetTexture::ShowDebugInfo() const
{
	Texture::ShowDebugInfo();

	ImGui::Indent(20.f);
	ImGui::Text("Runtime UAVID: %d", m_un64RuntimeUAVID);

	ImGui::SeparatorText("UAVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dUAVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::UAVViewDimensionsToString(m_d3dUAVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dUAVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipSlice : %d", m_d3dUAVDesc.Texture2D.MipSlice);
			ImGui::Text("Texture2D.PlaneSlice : %d", m_d3dUAVDesc.Texture2D.PlaneSlice);
		}
		else if (m_d3dUAVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) {
			ImGui::Text("Texture2DArray.MipSlice : %d", m_d3dUAVDesc.Texture2DArray.MipSlice);
			ImGui::Text("Texture2DArray.PlaneSlice : %d", m_d3dUAVDesc.Texture2DArray.PlaneSlice);
			ImGui::Text("Texture2DArray.ArraySize : %d", m_d3dUAVDesc.Texture2DArray.ArraySize);
			ImGui::Text("Texture2DArray.FirstArraySlice : %d", m_d3dUAVDesc.Texture2DArray.FirstArraySlice);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Text("Runtime RTVID: %d", m_un64RuntimeRTVID);

	ImGui::SeparatorText("RTVDesc");
	{
		ImGui::Text("DXGI format : %s", ::DXGIFormatToString(m_d3dRTVDesc.Format).c_str());
		ImGui::Text("ViewDimension : %s", ::RTVViewDimensionsToString(m_d3dRTVDesc.ViewDimension).c_str());

		ImGui::Indent(20.f);
		if (m_d3dRTVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D) {
			ImGui::Text("Texture2D.MipSlice : %d", m_d3dRTVDesc.Texture2D.MipSlice);
			ImGui::Text("Texture2D.PlaneSlice : %d", m_d3dRTVDesc.Texture2D.PlaneSlice);
		}
		else if (m_d3dRTVDesc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) {
			ImGui::Text("Texture2DArray.MipSlice : %d", m_d3dRTVDesc.Texture2DArray.MipSlice);
			ImGui::Text("Texture2DArray.PlaneSlice : %d", m_d3dRTVDesc.Texture2DArray.PlaneSlice);
			ImGui::Text("Texture2DArray.ArraySize : %d", m_d3dRTVDesc.Texture2DArray.ArraySize);
			ImGui::Text("Texture2DArray.FirstArraySlice : %d", m_d3dRTVDesc.Texture2DArray.FirstArraySlice);
		}
		ImGui::Unindent(20.f);
	}

	ImGui::Unindent(20.f);
}
