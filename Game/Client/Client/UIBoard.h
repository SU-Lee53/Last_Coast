#pragma once
#include "UIComponent.h"

class UIBoard {
public:
	// 0번이 가장 위, g_unUILayers - 1 번이 가장 아래
	// 그려지는 순서는 g_unUILayers - 1 이 처음, 0이 마지막
	constexpr static uint32 g_unUILayers = 3;
	using UIComponentLayer = std::array<std::vector<std::shared_ptr<IUIComponent>>, g_unUILayers>;

	void InsertText(const std::shared_ptr<IUIComponent>& pComponent);
	void RemoveText(const std::shared_ptr<IUIComponent>& pComponent);

	void InsertSprite(const std::shared_ptr<IUIComponent>& pComponent);
	void RemoveSprite(const std::shared_ptr<IUIComponent>& pComponent);

	void Update();

public:
	const UIComponentLayer& GetSpriteLayers() const { return m_SpriteLayers; }
	const UIComponentLayer& GetTextLayers() const { return m_TextLayers; }


private:
	UIComponentLayer m_SpriteLayers;
	UIComponentLayer m_TextLayers;
	

};

