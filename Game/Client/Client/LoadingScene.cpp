#include "pch.h"
#include "LoadingScene.h"

#include "DebugPlayer.h"
#include "TextBox.h"

void LoadingScene::BuildObjects()
{
	m_pPlayer = std::make_shared<DebugPlayer>();

	m_pUIBoard = std::make_unique<UIBoard>();

	m_pLoadingText = std::make_shared<TextBox>(L"Noto Sans KR");
	m_pLoadingText->SetText(L"Loading");
	m_pLoadingText->SetLayer(0);
	m_pLoadingText->SetAnchor(Vector2{ 0.5f, 0.5f });
	m_pLoadingText->SetPivot(Vector2{ 0.5f, 0.5f });
	m_pLoadingText->SetPosition(Vector2{ 0.f, 0.f });
	m_pLoadingText->SetTextHeight(70.f);

	m_pUIBoard->InsertUI(m_pLoadingText);
}

void LoadingScene::Update()
{
	m_fDotAnimationTime += DT;

	constexpr float fDotInterval = 0.35f;
	const uint32 unDotCount =
		static_cast<uint32>(m_fDotAnimationTime / fDotInterval) % 4;

	if (unDotCount == m_unCurrentDots) {
		return;
	}

	m_unCurrentDots = unDotCount;

	m_pLoadingText->SetText(std::wstring{ L"Loading" } + std::wstring(unDotCount, L'.'));
}
