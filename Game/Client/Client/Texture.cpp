#include "pch.h"
#include "Texture.h"

bool Texture::CreateTextureFromFile(const std::wstring& wstrTextureName)
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

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = m_pd3dResource->GetDesc();
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

void Texture::StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dBeforeState, D3D12_RESOURCE_STATES d3dAfterState)
{
	pd3dCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_pd3dResource.Get(),
			d3dBeforeState,
			d3dAfterState,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_BARRIER_FLAG_NONE)
	);

	m_d3dCurrentState = d3dAfterState;
}

bool RenderTargetTexture::Initialize(uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat)
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

	float pfClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = d3dRTTextureDesc.Format;
	::memcpy(clearValue.Color, pfClearColor, 4 * sizeof(float));
	
	DEVICE->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&d3dRTTextureDesc,
		D3D12_RESOURCE_STATE_PRESENT,
		&clearValue,
		IID_PPV_ARGS(m_pd3dResource.GetAddressOf())
	);


	m_d3dCurrentState = D3D12_RESOURCE_STATE_PRESENT;
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
	// TODO : 구현
	return true;
}
