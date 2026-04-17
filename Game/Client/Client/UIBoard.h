#pragma once
#include "UIComponent.h"

class UIBoard {
public:
	constexpr static uint32 g_unUILayers = 3;
	using UIComponentLayer = std::array<std::vector<UIComponent>, g_unUILayers>



public:

private:
	std::array<std::vector<UIComponent>, g_unUILayers> m_Layers;
	

};

