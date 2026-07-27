#pragma once

class D3DCore {
public:
	D3DCore(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync);
	~D3DCore();

public:
	void Initialize();

private:
	void CreateD3DDevice();

	bool CheckSupportSSE2();
	bool CheckSupportAVX2();

public:
	ComPtr<ID3D12Device> GetDevice() const;
	ComPtr<IDXGIFactory4> GetDXGIFactory() const;

private:
	ComPtr<IDXGIFactory4>	m_pdxgiFactory = nullptr;
	ComPtr<IDXGISwapChain3> m_pdxgiSwapChain = nullptr;
	ComPtr<ID3D12Device>	m_pd3dDevice = nullptr;

public:
	static uint32 GetDescriptorIncrementSize(DESCRIPTOR_TYPE);

	// 디버거 미부착 테스트 PC에서도 사후 수거 가능하도록 exe 옆 crash_log.txt에 append (+디버그 출력)
	static void AppendCrashLog(const std::string& strText);

	static uint32 g_nCBVSRVDescriptorIncrementSize;
	static uint32 g_nRTVDescriptorIncrementSize;
	static uint32 g_nDSVDescriptorIncrementSize;
	static uint32 g_nSamplerDescriptorIncrementSize;

	static uint32 g_unSyncInterval;

	static bool g_bEnableDebugLayer;
	static bool g_bEnableGBV;
	static bool g_bMsaa4xEnable;
	static bool g_nMsaa4xQualityLevels;

	static bool g_bSupportSSE2;
	static bool g_bSupportAVX2;

};
