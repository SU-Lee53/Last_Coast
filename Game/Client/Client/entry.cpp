#include "pch.h"
#include "resource.h"

//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(1084);
#endif _DEBUG

	WinCore* pApp = nullptr;

	// "-gpudebug" 인자: Release에서도 디버그 레이어+GBV 활성화 (Device Removed 조사용, 매우 느려짐)
	const BOOL bGpuDebug = (lpCmdLine && ::strstr(lpCmdLine, "-gpudebug") != nullptr) ? TRUE : FALSE;

#ifdef _DEBUG
	pApp = new WinCore(hInstance, 1600, 900, TRUE, TRUE, FALSE);
#else
	pApp = new WinCore(hInstance, 1600, 900, bGpuDebug, bGpuDebug, FALSE);
#endif

	pApp->Run();

	delete pApp;

#ifdef _DEBUG
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
#endif _DEBUG

}
