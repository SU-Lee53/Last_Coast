#include "pch.h"
#include "GuiManager.h"

HANDLE GuiManager::g_NewFrameEvent;

void GuiManager::Initialize(ComPtr<ID3D12Device> pd3dDevice)
{
	m_pFontSrvDescriptorHeap = std::make_unique<DescriptorHeap>();
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_pFontSrvDescriptorHeap->Initialize(pd3dDevice, desc);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(WinCore::g_hWnd);
	ImGui_ImplDX12_Init(D3DCORE->GetDevice().Get(), D3DCore::g_nCBVSRVDescriptorIncrementSize,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_pFontSrvDescriptorHeap->GetD3DDescriptorHeap().Get(),
		m_pFontSrvDescriptorHeap->GetDescriptorHandleFromHeapStart().cpuHandle,
		m_pFontSrvDescriptorHeap->GetDescriptorHandleFromHeapStart().gpuHandle);
}

void GuiManager::Update()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	if (INPUT->GetButtonDown(VK_F1)) {
		m_bShowDebugMenu = !m_bShowDebugMenu;
	}

	if (INPUT->GetButtonDown(VK_F3)) {
		m_bDraw = !m_bDraw;
	}

	if (m_bShowDebugMenu) {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Managers")) {

				if (ImGui::MenuItem("Render")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::RENDER_MANAGER) ? MANAGER_DEBUG::RENDER_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Scene")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::SCENE_MANAGER) ? MANAGER_DEBUG::SCENE_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Animation")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::ANIMATION_MANAGER) ? MANAGER_DEBUG::ANIMATION_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Texture")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::TEXTURE_MANAGER) ? MANAGER_DEBUG::TEXTURE_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Material")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::MATERIAL_MANAGER) ? MANAGER_DEBUG::MATERIAL_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Model")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::MODEL_MANAGER) ? MANAGER_DEBUG::MODEL_MANAGER : MANAGER_DEBUG::NONE;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

	// 채팅 창 토글 (F2)
	if (INPUT->GetButtonDown(VK_F2)) {
		m_bShowChat = !m_bShowChat;
	}

	DrawChatWindow();

	switch (m_eManagerDebug) {
	case MANAGER_DEBUG::RENDER_MANAGER:
	{
		RENDER->ShowDebugOptions();
		break;
	}
	case MANAGER_DEBUG::SCENE_MANAGER:
	{
		SCENE->ShowDebugOptions();
		break;
	}
	case MANAGER_DEBUG::ANIMATION_MANAGER:
	{
		ANIMATION->ShowDebugOptions();
		break;
	}
	case MANAGER_DEBUG::TEXTURE_MANAGER:
	{
		RENDER->ShowDebugOptions();
		break;
	}
	case MANAGER_DEBUG::MATERIAL_MANAGER:
	{
		RENDER->ShowDebugOptions();
		break;
	}
	case MANAGER_DEBUG::MODEL_MANAGER:
	{
		RENDER->ShowDebugOptions();
		break;
	}
	}


}

void GuiManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
	if (m_bDraw) {
		ImGui::Render();
		pd3dCommandList->SetDescriptorHeaps(1, m_pFontSrvDescriptorHeap->GetD3DDescriptorHeap().GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pd3dCommandList.Get());
	}
}

void GuiManager::DrawChatWindow()
{
	// 1) 네트워크 스레드가 큐에 넣은 수신 메시지를 메인 스레드에서 소비
	for (const ChatMessageEvent& ev : NETWORK->ConsumeChatMessages()) {
		std::string line = "[" + ev.username + "] " + ev.message;
		m_ChatHistory.push_back(line);
		if (m_ChatHistory.size() > MAX_CHAT_HISTORY)
			m_ChatHistory.erase(m_ChatHistory.begin());
		m_bChatScrollToBottom = true;
	}

	if (!m_bShowChat) return;

	ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(20, 400), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Chat (F2)", &m_bShowChat)) {

		// 연결 상태 표시
		if (NETWORK->IsOffline() || !NETWORK->IsConnected()) {
			ImGui::TextDisabled("Offline - chat works only when connected online.");
		}

		// 메시지 히스토리 (입력줄 높이만큼 남기고 스크롤 영역)
		const float fFooterHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		if (ImGui::BeginChild("ChatScroll", ImVec2(0, -fFooterHeight), true,
			ImGuiWindowFlags_HorizontalScrollbar)) {
			for (const std::string& line : m_ChatHistory) {
				ImGui::TextWrapped("%s", line.c_str());
			}
			if (m_bChatScrollToBottom) {
				ImGui::SetScrollHereY(1.0f);
				m_bChatScrollToBottom = false;
			}
		}
		ImGui::EndChild();

		// 입력줄 — Enter 로 전송. 직전 프레임에 전송했다면 입력창에 다시 포커스
		if (m_bChatReclaimFocus) {
			ImGui::SetKeyboardFocusHere(0); // 다음 위젯(InputText)에 포커스
			m_bChatReclaimFocus = false;
		}
		ImGui::PushItemWidth(-60.0f);
		bool bSend = ImGui::InputText("##ChatInput", m_ChatInputBuf, sizeof(m_ChatInputBuf),
			ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Send")) bSend = true;

		if (bSend && m_ChatInputBuf[0] != '\0') {
			NETWORK->SendChat(m_ChatInputBuf);
			m_ChatInputBuf[0] = '\0';
			m_bChatReclaimFocus = true; // 다음 프레임에 입력창 재포커스
		}
	}
	ImGui::End();
}

void GuiManager::HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}
