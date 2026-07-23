#pragma once
#include "GameFramework.h"

class WinCore {
public:
	WinCore(HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight, BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync);

	void Run();

public:
	static void RequestResolution(DWORD dwWidth, DWORD dwHeight);
	static void RequestToggleFullscreen();
	static bool IsBorderlessFullscreen() { return g_bBorderlessFullscreen; }

private:
	ATOM MyRegisterClass();
	BOOL InitInstance(int cmdShow);
	static LRESULT CALLBACK WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static void SetWindowedResolution(DWORD dwWidth, DWORD dwHeight);
	static void ToggleBorderlessFullscreen();
	static void ResizeClientResources();

public:
	static HINSTANCE sm_hInstance;
	static HWND g_hWnd;

	static DWORD g_dwClientWidth;
	static DWORD g_dwClientHeight;

	static bool g_bBorderlessFullscreen;
	static LONG_PTR g_lpWindowedStyle;
	static WINDOWPLACEMENT g_WindowedPlacement;

public:
	std::wstring m_wstrGameName = L"";
	std::shared_ptr<GameFramework> m_pGameFramework = nullptr;

};

