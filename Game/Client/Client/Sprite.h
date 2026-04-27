#pragma once
#include "UIComponent.h"

interface IImageSprite abstract{
public:
	IImageSprite(const std::string & strTexturePath);

protected:
	TextureRef<Texture> m_TextureHandle{};

};

class ImageBox : public IUIComponent, public IImageSprite {
public:
	ImageBox(const std::string& strTexturePath) : IImageSprite{ strTexturePath } {
		m_v4Color = Vector4(1.f, 1.f, 1.f, 1.f);
	}
	virtual const TextureRef<Texture>& GetTextureRef() const override { return m_TextureHandle; }
	virtual UIRectData MakeSBData() const override;

};

class ImageButton : public IUIButtonComponent, public IImageSprite {
public:
	ImageButton(const std::string& strTexturePath) : IImageSprite{ strTexturePath } {}
	virtual const TextureRef<Texture>& GetTextureRef() const override { return m_TextureHandle; }
	virtual UIRectData MakeSBData() const override;
};

class Crosshair : public ImageBox {
public:
	Crosshair() : ImageBox("Crosshair") {
		m_v2Anchor = Vector2{ 0,0 };
		m_v2Pivot = Vector2{0.5f, 0.5f};
		m_v2Position = Vector2{
			static_cast<float>(WinCore::g_dwClientWidth) / 2.0f,
			static_cast<float>(WinCore::g_dwClientHeight) / 2.0f
		};
		m_v2Size = Vector2{ 100.f,100.f };

		m_unLayer = 0;
		m_bVisible = false;
	}
};
