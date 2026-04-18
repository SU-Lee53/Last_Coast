#pragma once
#include "UIComponent.h"

class Sprite : public IUIComponent {
public:
	const TextureRef<Texture>& GetTextureRef() const { return m_TextureHandle; }
	
	virtual UIRectData MakeSBData() const override;



protected:
	TextureRef<Texture> m_TextureHandle{};

};
