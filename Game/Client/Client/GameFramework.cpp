#include "pch.h"
#include "GameFramework.h"
#include "Scene.h"

std::unique_ptr<D3DCore>			GameFramework::g_pD3DCore = nullptr;
std::unique_ptr<ResourceManager>	GameFramework::g_pResourceManager = nullptr;
std::unique_ptr<RenderManager>		GameFramework::g_pRenderManager = nullptr;
std::unique_ptr<ShaderManager>		GameFramework::g_pShaderManager = nullptr;
std::unique_ptr<TextureManager>		GameFramework::g_pTextureManager = nullptr;
std::unique_ptr<SceneManager>		GameFramework::g_pSceneManager = nullptr;
std::unique_ptr<InputManager>		GameFramework::g_pInputManager = nullptr;
std::unique_ptr<GameTimer>			GameFramework::g_pGameTimer = nullptr;
std::unique_ptr<ModelManager>		GameFramework::g_pModelManager = nullptr;
std::unique_ptr<GuiManager>			GameFramework::g_pGuiManager = nullptr;
std::unique_ptr<NetworkManager>		GameFramework::g_pNetworkManager = nullptr;
std::unique_ptr<EffectManager>		GameFramework::g_pEffectManager = nullptr;
std::unique_ptr<SoundManager>		GameFramework::g_pSoundManager = nullptr;
std::unique_ptr<UIManager>		GameFramework::g_pUIManager = nullptr;

GameFramework::GameFramework(BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	g_pD3DCore = std::make_unique<D3DCore>(bEnableDebugLayer, bEnableGBV);

	// Init managers
	g_pResourceManager = std::make_unique<ResourceManager>(g_pD3DCore->GetDevice());
	g_pShaderManager = std::make_unique<ShaderManager>(g_pD3DCore->GetDevice());
	g_pTextureManager = std::make_unique<TextureManager>(g_pD3DCore->GetDevice());
	g_pModelManager = std::make_unique<ModelManager>(g_pD3DCore->GetDevice());
	g_pRenderManager = std::make_unique<RenderManager>(g_pD3DCore->GetDevice(), g_pD3DCore->GetCommandList());
	g_pSceneManager = std::make_unique<SceneManager>();
	g_pInputManager = std::make_unique<InputManager>(WinCore::g_hWnd);
	g_pGameTimer = std::make_unique<GameTimer>();
	g_pGuiManager = std::make_unique<GuiManager>(g_pD3DCore->GetDevice());
	g_pNetworkManager = std::make_unique<NetworkManager>();
	g_pEffectManager = std::make_unique<EffectManager>();
	g_pSoundManager = std::make_unique<SoundManager>();

	g_pShaderManager->Initialize();
	g_pEffectManager->Initialize(g_pD3DCore->GetDevice(), g_pD3DCore->GetCommandList());

	g_pUIManager = std::make_unique<UIManager>(g_pD3DCore->GetDevice());

	g_pTextureManager->LoadGameTextures();
	g_pModelManager->LoadGameModels();

	g_pSceneManager->Initialize();

	g_pShaderManager->ReleaseBlobs();

	g_pSoundManager->Initialize();

	g_pGameTimer->Start();

	// Init Scene
	//m_pScene = std::make_shared<TestScene>();
	//m_pScene->BuildObjects();
	//g_pResourceManager->ExcuteCommandList();
}

void GameFramework::Update()
{
	g_pGameTimer->Tick();
	g_pGuiManager->Update();
	g_pSoundManager->Update();

	g_pRenderManager->Clear();
	g_pUIManager->Clear();

	g_pInputManager->Update();
	g_pSceneManager->ProcessInput();
	g_pSceneManager->Update();

	g_pEffectManager->Update(DT);
}

void GameFramework::Render()
{
	g_pD3DCore->RenderBegin();

	// TODO : Render Logic Here
	g_pRenderManager->Render(g_pD3DCore->GetCommandList());
	g_pSceneManager->Render(g_pD3DCore->GetCommandList());		// 별도의 렌더링 방법을 갖는다면 여기서 렌더링 함
	g_pEffectManager->Render(g_pD3DCore->GetCommandList());
	g_pUIManager->Render(g_pD3DCore->GetCommandList());
	g_pGuiManager->Render(g_pD3DCore->GetCommandList());

	g_pD3DCore->RenderEnd();

	g_pResourceManager->ResetCBufferBool();

	g_pD3DCore->Present();
	g_pD3DCore->MoveToNextFrame();


	std::wstring tstrFrameRate;
	g_pGameTimer->GetFrameRate(L"Game", tstrFrameRate);
	tstrFrameRate = std::format(L"{}", tstrFrameRate);
	::SetWindowText(WinCore::g_hWnd, tstrFrameRate.data());

}

void GameFramework::CleanUp()
{
	SCENE->CleanUp();
}
