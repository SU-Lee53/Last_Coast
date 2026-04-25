#include "pch.h"
#include "Sprite.h"

Sprite::Sprite(const std::string& strTexturePath)
{
	m_v4Color = Vector4(1.f, 1.f, 1.f, 1.f);
	m_TextureHandle = TEXTURE->LoadTexture(strTexturePath);
}

UIRectData Sprite::MakeSBData() const
{
	RECT screenRect = GetScreenRect();

	UIRectData data;
	data.v4ScreenRect.x = static_cast<float>(screenRect.left);
	data.v4ScreenRect.y = static_cast<float>(screenRect.top);
	data.v4ScreenRect.z = static_cast<float>(screenRect.right);
	data.v4ScreenRect.w = static_cast<float>(screenRect.bottom);

	data.v4UVRect.x = 0.f;
	data.v4UVRect.y = 0.f;
	data.v4UVRect.z = 1.f;
	data.v4UVRect.w = 1.f;

	data.v4Color = m_v4Color;
	//data.v4TextColorOrTexIndex = m_v4TextColor;

	return data;
}
