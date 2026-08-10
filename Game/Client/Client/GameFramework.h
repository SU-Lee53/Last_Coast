#pragma once
#include "Scene.h"
#include "D3DCore.h"
#include "GameContext.h"

class GameFramework {
public:
	GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync);
	~GameFramework();

	void Update();
	void Render();

	void CleanUp();

private:
	//std::unique_ptr<Scene> m_pScene = nullptr;
	bool m_bCleanedUp = false;

public:
	// Core & Managers
	static std::unique_ptr<D3DCore>				g_pD3DCore;
	static std::unique_ptr<GameContext>			g_GameContext;
};

#define D3DCORE					GameFramework::g_pD3DCore
#define GCTX					GameFramework::g_GameContext

#define DEVICE					D3DCORE->GetDevice()
#define DXGI_FACTORY			D3DCORE->GetDXGIFactory()
