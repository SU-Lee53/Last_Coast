#include "pch.h"
#include "D3DCore.h"

UINT D3DCore::g_nCBVSRVDescriptorIncrementSize = 0;
UINT D3DCore::g_nRTVDescriptorIncrementSize = 0;
UINT D3DCore::g_nDSVDescriptorIncrementSize = 0;
UINT D3DCore::g_nSamplerDescriptorIncrementSize = 0;
uint32 D3DCore::g_unSyncInterval = 0;

bool D3DCore::g_bEnableDebugLayer = false;
bool D3DCore::g_bEnableGBV = false;
bool D3DCore::g_bMsaa4xEnable = false;
bool D3DCore::g_nMsaa4xQualityLevels = 0;

D3DCore::D3DCore(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync)
{
	g_bEnableDebugLayer = bEnableDebugLayer;
	g_bEnableGBV = bEnableGBV;
	g_unSyncInterval = (bEnableVSync) ? 1 : 0;
}

D3DCore::~D3DCore()
{
	//WaitForGPUComplete();
	//RENDER->WaitForGPUComplete();
}

void D3DCore::Initialize()
{
	CreateD3DDevice();

	g_nCBVSRVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_nRTVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_nDSVDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_nSamplerDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void D3DCore::CreateD3DDevice()
{
	HRESULT hr;

	UINT nDXGIFactoryFlags = 0;
	if (g_bEnableDebugLayer) {
		ComPtr<ID3D12Debug> pd3dDebugController = nullptr;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(pd3dDebugController.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("D3D12GetDebugInterface failed");
		}
		nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

		if (pd3dDebugController) {
			pd3dDebugController->EnableDebugLayer();
		}
	}

	// DXGI Factory
	hr = CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(m_pdxgiFactory.GetAddressOf()));
	if (FAILED(hr)) {
		SHOW_ERROR("CreateDXGIFactory2 failed");
	}

	// Create DXGIAdapter
	ComPtr<IDXGIAdapter> pdxgiAdapter = nullptr;
	DXGI_ADAPTER_DESC1 adapterDesc{};
	size_t bestMemory = 0;
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		ComPtr<IDXGIAdapter1> pCurAdapter = nullptr;
		if (m_pdxgiFactory->EnumAdapters1(adapterIndex, pCurAdapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) break;

		DXGI_ADAPTER_DESC1 curAdapterDesc{};
		if (FAILED(pCurAdapter->GetDesc1(&curAdapterDesc))) __debugbreak();

		size_t curMemory = curAdapterDesc.DedicatedVideoMemory / (1024 * 1024);

		if (curMemory > bestMemory)
		{
			bestMemory = curMemory;
			pdxgiAdapter = pCurAdapter;
			adapterDesc = curAdapterDesc;
		}
	}

	std::array<D3D_FEATURE_LEVEL, 3> featureLevels =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	// Device
	for (int i = 0; i < featureLevels.size(); i++) {
		hr = D3D12CreateDevice(pdxgiAdapter.Get(), featureLevels[i], IID_PPV_ARGS(m_pd3dDevice.GetAddressOf()));
		if (SUCCEEDED(hr))
			break;
	}

	if (!m_pd3dDevice) {
		m_pdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(pdxgiAdapter.GetAddressOf()));
		hr = D3D12CreateDevice(pdxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pd3dDevice.GetAddressOf()));
		if (FAILED(hr)) {
			SHOW_ERROR("D3D12CreateDevice failed");
		}
	}

	// MSAA
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	{
		d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3dMsaaQualityLevels.SampleCount = 4;
		d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		d3dMsaaQualityLevels.NumQualityLevels = 0;
	}

	hr = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	if (SUCCEEDED(hr)) {
		g_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
		g_bMsaa4xEnable = (g_nMsaa4xQualityLevels > 1) ? true : false;
	}
	else {
		::OutputDebugString(L"Device doesn't support MSAA4x");
	}

	// Fence
	//	hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pd3dFence.GetAddressOf()));
	//	if (FAILED(hr)) {
	//		SHOW_ERROR("Failed to create fence");
	//	}
	//	
	//	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

}

ComPtr<ID3D12Device> D3DCore::GetDevice() const
{
	return m_pd3dDevice;
}

ComPtr<IDXGIFactory4> D3DCore::GetDXGIFactory() const
{
	return m_pdxgiFactory;
}

uint32 D3DCore::GetDescriptorIncrementSize(DESCRIPTOR_TYPE eDescType)
{
	uint32 unDescriptorInc = 0;

	switch (eDescType)
	{
	case DESCRIPTOR_TYPE::CBV:
	case DESCRIPTOR_TYPE::SRV:
	case DESCRIPTOR_TYPE::UAV:
	{
		unDescriptorInc = g_nCBVSRVDescriptorIncrementSize;
		break;
	}
	case DESCRIPTOR_TYPE::RTV:
	{
		unDescriptorInc = g_nRTVDescriptorIncrementSize;
		break;
	}
	case DESCRIPTOR_TYPE::DSV:
	{
		unDescriptorInc = g_nDSVDescriptorIncrementSize;
		break;
	}
	case DESCRIPTOR_TYPE::SAMPLER:
	{
		unDescriptorInc = g_nSamplerDescriptorIncrementSize;
		break;
	}
	default:
		std::unreachable();
	}

	return unDescriptorInc;
}
