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

	if (m_bShowDebugMenu) {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Managers")) {

				if (ImGui::MenuItem("Render")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::RENDER_MANAGER) ? MANAGER_DEBUG::RENDER_MANAGER : MANAGER_DEBUG::NONE;
				}

				if (ImGui::MenuItem("Scene")) {
					m_eManagerDebug = (m_eManagerDebug != MANAGER_DEBUG::SCENE_MANAGER) ? MANAGER_DEBUG::SCENE_MANAGER : MANAGER_DEBUG::NONE;
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
    ImGui::Render();

    pd3dCommandList->SetDescriptorHeaps(1, m_pFontSrvDescriptorHeap->GetD3DDescriptorHeap().GetAddressOf());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pd3dCommandList.Get());

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
