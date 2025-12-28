#include "pch.h"
#include "GuiManager.h"

HANDLE GuiManager::g_NewFrameEvent;

GuiManager::GuiManager(ComPtr<ID3D12Device> pd3dDevice)
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

}

void GuiManager::Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommandList)
{
    ImGui::Render();

    pd3dCommandList->SetDescriptorHeaps(1, m_pFontSrvDescriptorHeap->GetD3DDescriptorHeap().GetAddressOf());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pd3dCommandList.Get());

}
