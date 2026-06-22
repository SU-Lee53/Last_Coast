#pragma once

class GuiManager {

	DECLARE_SINGLE(GuiManager)

	enum class MANAGER_DEBUG {
		RENDER_MANAGER = 0,
		SCENE_MANAGER,
		ANIMATION_MANAGER,
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
	// 채팅 창 — 수신 메시지 폴링 + ImGui 입력/히스토리 렌더
	void DrawChatWindow();

private:
	std::unique_ptr<DescriptorHeap> m_pFontSrvDescriptorHeap = nullptr;

public:
	static HANDLE g_NewFrameEvent;

	bool m_bShowDebugMenu = true;
	MANAGER_DEBUG m_eManagerDebug = MANAGER_DEBUG::NONE;

	// ── 채팅 ────────────────────────────────────────────────────────────────
	bool                     m_bShowChat = true;
	std::vector<std::string> m_ChatHistory;          // 누적된 채팅 라인
	char                     m_ChatInputBuf[200] = {}; // InputText 버퍼
	bool                     m_bChatScrollToBottom = false;
	bool                     m_bChatReclaimFocus = false;
	static constexpr size_t  MAX_CHAT_HISTORY = 100;
};

