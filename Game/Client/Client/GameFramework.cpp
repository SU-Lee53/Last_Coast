#include "pch.h"
#include "GameFramework.h"
#include "Scene.h"

#include "MuzzleFlashEffect.h"

std::unique_ptr<D3DCore> GameFramework::g_pD3DCore = nullptr;
std::unique_ptr<GameContext> GameFramework::g_GameContext = nullptr;

GameFramework::GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync)
{
	g_pD3DCore = std::make_unique<D3DCore>(bEnableDebugLayer, bEnableGBV, bEnableVSync);
	g_pD3DCore->Initialize();

	std::string strNavMesh = "GAME";
	// Init managers
	LOADING->Initialize();
	AI->Initialize(strNavMesh);
	COMPUTE->Initialize();
	RESOURCE->Initialize(g_pD3DCore->GetDevice());
	TEXTURE->Initialize(g_pD3DCore->GetDevice());
	MATERIAL->Initialize();
	RENDER->Initialize(g_pD3DCore->GetDevice());
	SHADER->Initialize(g_pD3DCore->GetDevice());

	//TEXT->Initialize(RENDER->GetCommandQueue());

	MODEL->Initialize();
	ANIMATION->Initialize();
	SOUND->Initialize();

	g_GameContext = std::make_unique<GameContext>();
	GCTX->Initialize();

	SCENE->Initialize();

	INPUT->Initialize(WinCore::g_hWnd);
	GUI->Initialize(g_pD3DCore->GetDevice());
	NETWORK->Initialize();
	PARTICLE->Initialize();
	//UI->Initialize(g_pD3DCore->GetDevice());

	RENDER->BuildRenderGraph();

	TEXTURE->LoadGameTextures();
	MODEL->LoadGameModels();
	ANIMATION->LoadGameAnimations();

	SHADER->ReleaseBlobs();

	TIME->Initialize();
	TIME->Start();
}

GameFramework::~GameFramework()
{
	RENDER->WaitForGPUComplete();
	LOADING->Shutdown();
}

void GameFramework::Update()
{
	TIME->Tick();
	SOUND->Update();
	GUI->Update();

	INPUT->Update();

	if (INPUT->GetButtonDown(VK_F9)) {
		WinCore::RequestToggleFullscreen();
	}

	const bool bWasAsyncLoading = SCENE->IsInAsyncSceneChanging();

	SCENE->ProcessInput();

	if (!bWasAsyncLoading) {
		AI->UpdateAll(DT);  // 에이전트 이동 먼저 → Zombie::PostUpdate에서 최신 위치 사용
	}

	SCENE->Update();

	const bool bIsAsyncLoading =
		SCENE->IsInAsyncSceneChanging();

	const bool bSceneTransitionFrame =
		bWasAsyncLoading || bIsAsyncLoading;

	if (!bSceneTransitionFrame) {
		SOUND->UpdateSoundQueue();
		PARTICLE->Update();
	}

	if (bIsAsyncLoading) {
		RESOURCE->PollCopyComplete();
		TEXTURE->PollCopyComplete();
	}
	else {
		RESOURCE->WaitForCopyComplete();
		TEXTURE->WaitForCopyComplete();
	}

	if (!bSceneTransitionFrame) {
		COMPUTE->ExecuteIndirect();
		COMPUTE->WaitForGPUComplete();
		COMPUTE->Reset();
	}
}

void GameFramework::Render()
{
	RENDER->Reset();

	SCENE->PrepareRender();
	RENDER->Render();
	
	std::wstring tstrFrameRate;
	TIME->GetFrameRate(L"Game", tstrFrameRate);
	tstrFrameRate = std::format(L"{}", tstrFrameRate);
	::SetWindowText(WinCore::g_hWnd, tstrFrameRate.data());
}

void GameFramework::CleanUp()
{
	SCENE->CleanUp();
}
