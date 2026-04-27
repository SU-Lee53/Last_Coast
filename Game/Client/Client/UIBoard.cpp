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

	for (auto& layer : m_UILayers) {
		for (auto& pComp : layer) {
			if (pComp->IsClickable()) {
				auto& pButton = static_pointer_cast<IUIButtonComponent>(pComp);
				bool bCursorOnButton = pButton->CheckPointInComponent(ptCursorPos);
				
				// Cursor is not on button -> Update and continue
				// If button was hovered -> EndHovered
				if (!bCursorOnButton) {
					if (pButton->WasHovered()) {
						pButton->OnEndHovered();
					}
					pComp->Update();
					continue;
				}

				// Cursor in on button -> Process it
				if (bClicked) {
					pButton->OnClicked();
				}
				else {
					if (!pButton->WasHovered()) {
						pButton->OnBeginHovered();
					}
				}
			}

			pComp->Update();
		}
	}

}
