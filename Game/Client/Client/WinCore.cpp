#include "pch.h"
#include "WinCore.h"
#include "Resource.h"

HINSTANCE WinCore::sm_hInstance = NULL;
HWND WinCore::g_hWnd = NULL;
DWORD WinCore::g_dwClientWidth = 0;
DWORD WinCore::g_dwClientHeight = 0;
bool WinCore::g_bBorderlessFullscreen = false;
LONG_PTR WinCore::g_lpWindowedStyle = 0;
WINDOWPLACEMENT WinCore::g_WindowedPlacement = { sizeof(WINDOWPLACEMENT) };

constexpr UINT WM_CHANGE_RESOLUTION = WM_APP + 1;
constexpr UINT WM_TOGGLE_BORDERLESS_FULLSCREEN = WM_APP + 2;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WinCore::WinCore(HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight, BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync)
{
    sm_hInstance = hInstance;
    g_dwClientWidth = dwWidth;
    g_dwClientHeight = dwHeight;
    m_wstrGameName = L"GAME NAME HERE";

    MyRegisterClass();

    if (!InitInstance(SW_SHOW)){
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        throw std::runtime_error("Failed to Initialize Application");
    }

    m_pGameFramework = std::make_shared<GameFramework>(bEnableDebugLayer, bEnableGBV, bEnableVSync);

}

void WinCore::Run()
{
    HACCEL hAccelTable = LoadAccelerators(sm_hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    MSG msg;

    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

		// Framework Update
		m_pGameFramework->Update();
		m_pGameFramework->Render();
    }

	RENDER->WaitForGPUComplete();

	// ImGui Clean Up
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void WinCore::RequestResolution(DWORD dwWidth, DWORD dwHeight)
{
	if (!g_hWnd || dwWidth == 0 || dwHeight == 0)
		return;

	PostMessageW(g_hWnd, WM_CHANGE_RESOLUTION, static_cast<WPARAM>(dwWidth), static_cast<LPARAM>(dwHeight));
}

void WinCore::RequestToggleFullscreen()
{
	if (!g_hWnd)
		return;

	PostMessageW(g_hWnd, WM_TOGGLE_BORDERLESS_FULLSCREEN, 0, 0);
}

ATOM WinCore::MyRegisterClass()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = sm_hInstance;
    wcex.hIcon = LoadIcon(sm_hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = m_wstrGameName.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL WinCore::InitInstance(int cmdShow)
{
    WCHAR szTitle[100];                  // 제목 표시줄 텍스트입니다.
    WCHAR szWindowClass[100];            // 기본 창 클래스 이름입니다.

    LoadStringW(sm_hInstance, IDS_APP_TITLE, szTitle, 100);
    LoadStringW(sm_hInstance, IDI_CLIENT, szWindowClass, 100);

    RECT rc = { 0,0,g_dwClientWidth, g_dwClientHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    g_hWnd = CreateWindowW(m_wstrGameName.c_str(), m_wstrGameName.c_str(), dwStyle, CW_USEDEFAULT, 0,
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, sm_hInstance, NULL);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, cmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

LRESULT WinCore::WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(handle, message, wParam, lParam))
        return true;

    switch (message)
    {

	case WM_KILLFOCUS:
	{
		::ClipCursor(nullptr);
		INPUT->ShowCursor();
		break;
	}

    case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}

	case WM_CHAR:
	{
		wchar_t ch = static_cast<wchar_t>(wParam);
		CUR_SCENE->GetUIBoard()->OnChar(ch);
		break;
	}

	case WM_CHANGE_RESOLUTION:
	{
		SetWindowedResolution(static_cast<DWORD>(wParam), static_cast<DWORD>(lParam));
		break;
	}

	case WM_TOGGLE_BORDERLESS_FULLSCREEN:
	{
		ToggleBorderlessFullscreen();
		break;
	}

    default:
        return DefWindowProc(handle, message, wParam, lParam);
    }
    return 0;
	
}

void WinCore::SetWindowedResolution(DWORD dwWidth, DWORD dwHeight)
{
	if (g_bBorderlessFullscreen || dwWidth == 0 || dwHeight == 0)
		return;

	RECT rc = { 0, 0, static_cast<LONG>(dwWidth), static_cast<LONG>(dwHeight) };
	DWORD dwStyle = static_cast<DWORD>(GetWindowLongPtrW(g_hWnd, GWL_STYLE));
	AdjustWindowRect(&rc, dwStyle, FALSE);

	const int nWindowWidth = rc.right - rc.left;
	const int nWindowHeight = rc.bottom - rc.top;

	MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
	if (!GetMonitorInfoW(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
		return;
	}

	const int x = monitorInfo.rcWork.left + (monitorInfo.rcWork.right - monitorInfo.rcWork.left - nWindowWidth) / 2;
	const int y = monitorInfo.rcWork.top + (monitorInfo.rcWork.bottom - monitorInfo.rcWork.top - nWindowHeight) / 2;

	SetWindowPos(g_hWnd, nullptr, x, y, nWindowWidth, nWindowHeight, SWP_NOZORDER | SWP_NOOWNERZORDER);
	ResizeClientResources();
}

void WinCore::ToggleBorderlessFullscreen()
{
	if (!g_bBorderlessFullscreen) {
		MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
		if (!GetMonitorInfoW(MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
			return;
		}

		g_WindowedPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(g_hWnd, &g_WindowedPlacement);

		g_lpWindowedStyle = GetWindowLongPtrW(g_hWnd, GWL_STYLE);
		LONG_PTR lBorderlessStyle = (g_lpWindowedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP;
		SetWindowLongPtrW(g_hWnd, GWL_STYLE, lBorderlessStyle);

		SetWindowPos(
			g_hWnd,
			HWND_TOP,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOOWNERZORDER
		);

		g_bBorderlessFullscreen = true;
	}
	else {
		SetWindowLongPtrW(g_hWnd, GWL_STYLE, g_lpWindowedStyle);
		SetWindowPlacement(g_hWnd, &g_WindowedPlacement);

		SetWindowPos(
			g_hWnd,
			nullptr,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);

		g_bBorderlessFullscreen = false;
	}

	ResizeClientResources();
}

void WinCore::ResizeClientResources()
{
	RECT rc;
	if (!GetClientRect(g_hWnd, &rc))
		return;

	const DWORD dwWidth = static_cast<DWORD>(rc.right - rc.left);
	const DWORD dwHeight = static_cast<DWORD>(rc.bottom - rc.top);

	if (dwWidth == 0 || dwHeight == 0)
		return;

	if (!RENDER->Resize(dwWidth, dwHeight))
		return;

	const std::shared_ptr<Camera>& pCamera = CUR_SCENE->GetCamera();
	if (pCamera)
		pCamera->Resize(dwWidth, dwHeight);
}
