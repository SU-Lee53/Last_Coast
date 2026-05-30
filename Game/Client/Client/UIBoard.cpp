#include "pch.h"
#include "UIBoard.h"
#include "TextBox.h"
#include "Sprite.h"

void UIBoard::InsertUI(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_UILayers[unLayer].push_back(pComponent);
}

void UIBoard::RemoveUI(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_UILayers[unLayer].erase(std::remove(m_UILayers[unLayer].begin(), m_UILayers[unLayer].end(), pComponent), m_UILayers[unLayer].end());
}

void UIBoard::Update()
{
	bool bClicked = INPUT->GetButtonDown(VK_LBUTTON);
	POINT ptCursorPos = INPUT->GetCurrentCursorPos();
	bool bFocusTargetFound = false;

	for (auto& layer : m_UILayers) {
		for (auto& pComp : layer) {
			if (pComp->IsClickable()) {
				//auto& pButton = static_pointer_cast<IUIButtonComponent>(pComp);
				bool bCursorOnButton = pComp->CheckPointInComponent(ptCursorPos);

				if (pComp->IsClickable()) {
					if (!bCursorOnButton) {
						if (pComp->WasHovered()) {
							pComp->OnEndHovered();
						}

						pComp->Update();
						continue;
					}

					if (bClicked) {
						pComp->OnClicked();
					}
					else {
						if (!pComp->WasHovered()) {
							pComp->OnBeginHovered();
						}
					}
				}

				if (bClicked && bCursorOnButton && pComp->IsFocusable()) {
					SetFocus(pComp);
					bFocusTargetFound = true;
				}

			}

			pComp->Update();
		}
	}

	if (bClicked && !bFocusTargetFound) {
		ClearFocus();
	}
}

void UIBoard::SetFocus(const std::shared_ptr<IUIComponent>& pFocus)
{
	if (auto pPrev = m_pCurrentFocused.lock()) {
		if (pPrev == pFocus)
			return;

		pPrev->SetFocused(false);
	}

	m_pCurrentFocused = pFocus;

	if (pFocus) {
		pFocus->SetFocused(true);
	}
}

void UIBoard::ClearFocus()
{
	if (auto pPrev = m_pCurrentFocused.lock()) {
		pPrev->SetFocused(false);
	}

	m_pCurrentFocused.reset();
}

void UIBoard::OnChar(wchar_t ch)
{
	if (auto pFocused = m_pCurrentFocused.lock()) {
		pFocused->OnChar(ch);
	}
}
