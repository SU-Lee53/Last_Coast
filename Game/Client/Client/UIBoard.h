#pragma once
#include "UIComponent.h"

class UIBoard {
public:
	// 0번이 가장 위, g_unUILayers - 1 번이 가장 아래
	// 그려지는 순서는 g_unUILayers - 1 이 처음, 0이 마지막
	constexpr static uint32 g_unUILayers = 3;
	using UIComponentLayer = std::array<std::vector<std::shared_ptr<IUIComponent>>, g_unUILayers>;

	void InsertUI(const std::shared_ptr<IUIComponent>& pComponent);
	void RemoveUI(const std::shared_ptr<IUIComponent>& pComponent);

	void Update();

	void SetFocus(const std::shared_ptr<IUIComponent>& pFocus);
	void ClearFocus();

	void OnChar(wchar_t ch);

public:
	const UIComponentLayer& GetUILayers() const { return m_UILayers; }


private:
	UIComponentLayer m_UILayers;
	//UIComponentLayer m_TextLayers;
	
	std::weak_ptr<IUIComponent> m_pCurrentFocused;

};

