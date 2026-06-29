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
}

void GameFramework::Update()
{
	TIME->Tick();
	SOUND->Update();
	GUI->Update();

	//UI->Clear();

	INPUT->Update();
	SCENE->ProcessInput();
	AI->UpdateAll(DT);   // 에이전트 이동 먼저 → Zombie::PostUpdate에서 최신 위치 사용
	SCENE->Update();

	SOUND->UpdateSoundQueue();
	PARTICLE->Update();

	// 게임 중간에 리소스 생성이 필요할 수 있으므로 대기
	// 리소스 생성될게 없으면 바로 리턴함
	RESOURCE->WaitForCopyComplete();
	TEXTURE->WaitForCopyComplete();

	COMPUTE->ExecuteIndirect();
	COMPUTE->WaitForGPUComplete();
	COMPUTE->Reset();
}

void GameFramework::Render()
{
	// TODO : Render Logic Here
	//RENDER->Render(g_pD3DCore->GetCommandList());
	//EFFECT->Render(g_pD3DCore->GetCommandList());
	//UI->Render(g_pD3DCore->GetCommandList());

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
