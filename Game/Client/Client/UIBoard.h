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

	void EditUI();

private:
	UIComponentLayer m_UILayers;
	//UIComponentLayer m_TextLayers;
	
	std::weak_ptr<IUIComponent> m_pCurrentFocused;

	// Editor variables
	int32 m_nSelectedLayer = 0;
	int32 m_nSelectedUIIndex = 0;

	std::string m_strNewName;
	int32 m_nNewLayer = 0;
	Vector2 m_v2NewAnchor = Vector2::Zero;
	Vector2 m_v2NewPivot = Vector2::Zero;
	Vector2 m_v2NewPosition = Vector2::Zero;
	float m_fNewWidth = 0.f;
	float m_fNewHeight = 0.f;
};

