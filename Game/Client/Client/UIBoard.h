#pragma once
#include "UIComponent.h"

class UIBoard {
public:
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

