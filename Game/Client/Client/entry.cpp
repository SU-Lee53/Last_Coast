#include "pch.h"
#include "resource.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetBreakAlloc(1084);
#endif _DEBUG

	WinCore* pApp = nullptr;

#ifdef _DEBUG
	pApp = new WinCore(hInstance, 800, 600, TRUE, TRUE);
#else
	pApp = new WinCore(hInstance, 800, 600, FALSE, FALSE);
#endif

	pApp->Run();

	delete pApp;

#ifdef _DEBUG
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
#endif _DEBUG

}
