#pragma once
#include "Scene.h"
#include "D3DCore.h"

class GameFramework {
public:
	GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync);

	void Update();
	void Render();

	void CleanUp();

private:
	//std::unique_ptr<Scene> m_pScene = nullptr;

public:
	// Core & Managers
	static std::unique_ptr<D3DCore>				g_pD3DCore;
};

#define D3DCORE			GameFramework::g_pD3DCore

#define DEVICE			D3DCORE->GetDevice()
