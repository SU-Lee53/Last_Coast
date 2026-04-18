#include "pch.h"
#include "UIBoard.h"
#include "TextBox.h"
#include "Sprite.h"

void UIBoard::InsertText(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_TextLayers[unLayer].push_back(pComponent);
}

void UIBoard::RemoveText(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_TextLayers[unLayer].erase(std::remove(m_TextLayers[unLayer].begin(), m_TextLayers[unLayer].end(), pComponent), m_TextLayers[unLayer].end());
}

void UIBoard::InsertSprite(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_SpriteLayers[unLayer].push_back(pComponent);
}

void UIBoard::RemoveSprite(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_SpriteLayers[unLayer].erase(std::remove(m_SpriteLayers[unLayer].begin(), m_SpriteLayers[unLayer].end(), pComponent), m_SpriteLayers[unLayer].end());
}

void UIBoard::Update()
{
	for (auto& layer : m_SpriteLayers) {
		for (auto& pComp : layer) {
			pComp->Update();
		}
	}

	for (auto& layer : m_TextLayers) {
		for (auto& pComp : layer) {
			pComp->Update();
		}
	}
}
