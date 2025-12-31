#pragma once
#include "Scene.h"
#include "D3DCore.h"

#include "RenderManager.h"
#include "ResourceManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ModelManager.h"
#include "GameTimer.h"
#include "GuiManager.h"
#include "NetworkManager.h"
#include "EffectManager.h"
#include "SoundManager.h"
#include "UIManager.h"

class GameFramework {
public:
	GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV);

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
