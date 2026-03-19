#include "pch.h"
#include "GameFramework.h"
#include "Scene.h"

std::unique_ptr<D3DCore> GameFramework::g_pD3DCore = nullptr;

GameFramework::GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync)
{
	g_pD3DCore = std::make_unique<D3DCore>(bEnableDebugLayer, bEnableGBV, bEnableVSync);
	g_pD3DCore->Initialize();
	
	// Init managers
	COMPUTE->Initialize();
	RESOURCE->Initialize(g_pD3DCore->GetDevice());
	TEXTURE->Initialize(g_pD3DCore->GetDevice());
	MATERIAL->Initialize();
	RENDER->Initialize(g_pD3DCore->GetDevice());
	SHADER->Initialize(g_pD3DCore->GetDevice());

	MODEL->Initialize();
	ANIMATION->Initialize();

	SCENE->Initialize();

	INPUT->Initialize(WinCore::g_hWnd);
	GUI->Initialize(g_pD3DCore->GetDevice());
	NETWORK->Initialize();
	EFFECT->Initialize(g_pD3DCore->GetDevice());
	//UI->Initialize(g_pD3DCore->GetDevice());

	RENDER->BuildRenderGraph();

	TEXTURE->LoadGameTextures();
	MODEL->LoadGameModels();
	ANIMATION->LoadGameAnimations();

	//SOUND->Initialize();

	SHADER->ReleaseBlobs();

	TIMER->Initialize();
	TIMER->Start();
}

GameFramework::~GameFramework()
{
	RENDER->WaitForGPUComplete();
}

void GameFramework::Update()
{
	TIMER->Tick();
	GUI->Update();
	SOUND->Update();

	//UI->Clear();

	INPUT->Update();
	SCENE->ProcessInput();
	SCENE->Update();

	EFFECT->Update(DT);

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

	SCENE->PrepareRender();
	RENDER->Render();
	
	std::wstring tstrFrameRate;
	TIMER->GetFrameRate(L"Game", tstrFrameRate);
	tstrFrameRate = std::format(L"{}", tstrFrameRate);
	::SetWindowText(WinCore::g_hWnd, tstrFrameRate.data());

}

void GameFramework::CleanUp()
{
	SCENE->CleanUp();
}
