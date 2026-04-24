#pragma once
#include "UIComponent.h"

class Sprite : public IUIComponent {
public:
	const TextureRef<Texture>& GetTextureRef() const { return m_TextureHandle; }
	
	virtual UIRectData MakeSBData() const override;

protected:
	TextureRef<Texture> m_TextureHandle{};

};

class Crosshair : public Sprite {
public:
	Crosshair() {
		m_v2Anchor = Vector2{ 0,0 };
		m_v2Pivot = Vector2{0.5f, 0.5f};
		m_v2Position = Vector2{
			static_cast<float>(WinCore::g_dwClientWidth) / 2.0f,
			static_cast<float>(WinCore::g_dwClientHeight) / 2.0f
		};
		m_v2Size = Vector2{ 100.f,100.f };

		m_TextureHandle = TEXTURE->LoadTexture("Crosshair");

		m_unLayer = 0;
		m_bVisible = false;
	}
};
