#pragma once

class GuiManager {

	DECLARE_SINGLE(GuiManager)

	enum class MANAGER_DEBUG {
		RENDER_MANAGER = 0,
		SCENE_MANAGER,
		TEXTURE_MANAGER,
		MATERIAL_MANAGER,
		MODEL_MANAGER,

		NONE
	};

public:
	void Initialize(ComPtr<ID3D12Device> pd3dDevice);
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList);

	static void HelpMarker(const char* desc);

private:
	std::unique_ptr<DescriptorHeap> m_pFontSrvDescriptorHeap = nullptr;

public:
	static HANDLE g_NewFrameEvent;

	bool m_bShowDebugMenu = true;
	MANAGER_DEBUG m_eManagerDebug = MANAGER_DEBUG::NONE;
};

