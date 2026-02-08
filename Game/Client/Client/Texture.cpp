#include "pch.h"
#include "Texture.h"

Texture::Texture()
{

}

void Texture::CreateTextureFromFile(const std::wstring& wstrTextureName)
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
		return;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subResources;
	if (texPath.extension().string() == ".dds" || texPath.extension().string() == ".DDS") {
		LoadFromDDSFile(m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);
	}
	else {
		LoadFromWICFile(m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);
	}

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT nSubResources = (UINT)subResources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;

	TEXTURE->UpdateResources(m_pTexResource, subResources, nBytes);
	m_d3dSRVHandle = TEXTURE->CreateSRV(m_pTexResource, nullptr, m_d3dSRVDesc);
}

void Texture::CreateTextureArrayFromFile(const std::wstring& wstrTexturePath)
{
	namespace fs = std::filesystem;

	fs::path texPath{ wstrTexturePath };
	if (!fs::exists(texPath)) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", texPath.string()).c_str());
		return;
	}

	std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

	std::unique_ptr<uint8_t[]> ddsData = nullptr;
	std::vector<D3D12_SUBRESOURCE_DATA> subResources;
	if (texPath.extension().string() == ".dds" || texPath.extension().string() == ".DDS") {
		LoadFromDDSFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);
	}
	else {
		LoadFromWICFile(pTexture->m_pTexResource.pResource.GetAddressOf(), wstrTexturePath, ddsData, subResources);
	}

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT nSubResources = (UINT)subResources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, nSubResources);
	nBytes = (nBytes == 0) ? 1 : nBytes;

	TEXTURE->UpdateResources(m_pTexResource, subResources, nBytes);
	m_d3dSRVHandle = TEXTURE->CreateSRV(m_pTexResource, nullptr, m_d3dSRVDesc);
}

void Texture::CreateTextureFromRawFile(const std::wstring& wstrTexturePath, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiFormat)
{
	namespace fs = std::filesystem;

	//std::wstring wstrTexturePath;
	std::ifstream in{ wstrTexturePath, std::ios::binary };
	if (!in) {
		OutputDebugStringA(std::format("{} - {} : {} : {}\n", __FILE__, __LINE__, "Texture file not exist", fs::path(wstrTexturePath).string()).c_str());
		__debugbreak();
		return;
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

	pTexture->m_pTexResource.Create(
		DEVICE,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr
	);

	// UploadBuffer 생성
	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pTexture->m_pTexResource.pResource->GetDesc();
	UINT64 nBytes = GetRequiredIntermediateSize(pTexture->m_pTexResource.pResource.Get(), 0, 1);

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

	TEXTURE->UpdateResources(m_pTexResource, subResources, nBytes, pd3dUploadBuffer);
	m_d3dSRVHandle = TEXTURE->CreateSRV(m_pTexResource, nullptr, m_d3dSRVDesc);
}

void Texture::LoadFromDDSFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
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
		return;
	}

}

void Texture::LoadFromWICFile(ID3D12Resource** ppOutResource, const std::wstring& wstrTexturePath, std::unique_ptr<uint8_t[]>& ddsData, std::vector<D3D12_SUBRESOURCE_DATA>& subResources)
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
		return;
	}
}

void Texture::StateTransition(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, D3D12_RESOURCE_STATES d3dAfterState)
{
	m_pTexResource.StateTransition(pd3dCommandList, d3dAfterState);
}

void RenderTargetTexture::Initialize(ComPtr<ID3D12Device> pd3dDevice, uint32 unWidth, uint32 unHeight, DXGI_FORMAT dxgiSRVFormat, DXGI_FORMAT dxgiRTVFormat)
{
	D3D12_RESOURCE_DESC d3dRTTextureDesc;
	{
		d3dRTTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dRTTextureDesc.Alignment = 0;
		d3dRTTextureDesc.Width = unWidth;
		d3dRTTextureDesc.Height = unHeight;
		d3dRTTextureDesc.DepthOrArraySize = 1;
		d3dRTTextureDesc.Format = DXGI_FORMAT_UNKNOWN;
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
	
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&d3dRTTextureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(m_pTexResource.pResource.GetAddressOf())
	);

	m_d3dSRVHandle = TEXTURE->CreateSRV(m_pTexResource, &dxgiSRVFormat, m_d3dSRVDesc);
	m_d3dRTVHandle = TEXTURE->CreateRTV(m_pTexResource, &dxgiRTVFormat, m_d3dRTVDesc);
}

void UnorderedAccessTexture::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight)
{
	// TODO : 구현
}

void DepthStencilTexture::Initialize(ComPtr<ID3D12Device> pd3dDevice, ComPtr<ID3D12GraphicsCommandList> pd3dCommandList, UINT nWidth, UINT nHeight)
{
	// TODO : 구현
}
