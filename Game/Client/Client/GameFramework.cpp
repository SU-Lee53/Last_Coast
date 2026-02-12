#include "pch.h"
#include "GameFramework.h"
#include "Scene.h"

std::unique_ptr<D3DCore> GameFramework::g_pD3DCore = nullptr;

GameFramework::GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV, BOOL bEnableVSync)
{
	g_pD3DCore = std::make_unique<D3DCore>(bEnableDebugLayer, bEnableGBV, bEnableVSync);
	
	// Init managers
	RESOURCE->Initialize(g_pD3DCore->GetDevice());
	MATERIAL->Initialize();
	RENDER->Initialize(g_pD3DCore->GetDevice(), g_pD3DCore->GetCommandList());
	SHADER->Initialize(g_pD3DCore->GetDevice());
	INPUT->Initialize(WinCore::g_hWnd);
	MODEL->Initialize();
	GUI->Initialize(g_pD3DCore->GetDevice());
	NETWORK->Initialize();
	TIME->Initialize();
	TEXTURE->Initialize(g_pD3DCore->GetDevice());
	EFFECT->Initialize(g_pD3DCore->GetDevice(), g_pD3DCore->GetCommandList());
	UI->Initialize(g_pD3DCore->GetDevice());
	ANIMATION->Initialize();

	TEXTURE->LoadGameTextures();
	MODEL->LoadGameModels();
	ANIMATION->LoadGameAnimations();

	SCENE->Initialize();

	//SOUND->Initialize();

	SHADER->ReleaseBlobs();

	RESOURCE->WaitForCopyComplete();
	TEXTURE->WaitForCopyComplete();

	TIMER->Initialize();
	TIMER->Start();

	// Init Scene
	//m_pScene = std::make_shared<TestScene>();
	//m_pScene->BuildObjects();
	//g_pResourceManager->ExcuteCommandList();
}

void GameFramework::Update()
{
	TIMER->Tick();
	GUI->Update();
	SOUND->Update();

	RENDER->Clear();
	UI->Clear();

	INPUT->Update();
	SCENE->ProcessInput();
	SCENE->Update();

	EFFECT->Update(DT);

	// 게임 중간에 리소스 생성이 필요할 수 있으므로 대기
	// 리소스 생성될게 없으면 바로 리턴함
	RESOURCE->WaitForCopyComplete();
	TEXTURE->WaitForCopyComplete();
}

void GameFramework::Render()
{
	g_pD3DCore->RenderBegin();

	// TODO : Render Logic Here
	SCENE->Render(g_pD3DCore->GetCommandList());
	RENDER->Render(g_pD3DCore->GetCommandList());
	EFFECT->Render(g_pD3DCore->GetCommandList());
	UI->Render(g_pD3DCore->GetCommandList());
	GUI->Render(g_pD3DCore->GetCommandList());

	g_pD3DCore->RenderEnd();

	RESOURCE->ResetCBufferBool();

	g_pD3DCore->Present();
	g_pD3DCore->MoveToNextFrame();


	std::wstring tstrFrameRate;
	TIMER->GetFrameRate(L"Game", tstrFrameRate);
	tstrFrameRate = std::format(L"{}", tstrFrameRate);
	::SetWindowText(WinCore::g_hWnd, tstrFrameRate.data());

}

void GameFramework::CleanUp()
{
	SCENE->CleanUp();
}
