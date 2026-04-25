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
	POINT ptCursorPos = (bClicked) ? INPUT->GetCurrentCursorPos() : POINT{0, 0};

	for (auto& layer : m_UILayers) {
		for (auto& pComp : layer) {
			if (bClicked) {
				if (pComp->IsClickable()) {
					if (pComp->CheckClicked(ptCursorPos)) {
						pComp->OnClicked();
					}
				}
			}

			pComp->Update();
		}
	}

}
