#pragma once

class GuiManager {

	DECLARE_SINGLE(GuiManager)

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

private:
	std::unique_ptr<DescriptorHeap> m_pFontSrvDescriptorHeap = nullptr;

public:
	static HANDLE g_NewFrameEvent;

};

