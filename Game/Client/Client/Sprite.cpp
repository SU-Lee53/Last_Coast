#include "pch.h"
#include "Sprite.h"

IImageSprite::IImageSprite(const std::string& strTexturePath)
{
	m_TextureHandle = TEXTURE->LoadTexture(strTexturePath);
}

void IImageSprite::SetTexture(const std::string& strTexturePath)
{
	m_TextureHandle = TEXTURE->LoadTexture(strTexturePath);
}

UIRectData ImageBox::MakeSBData() const
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
	data.fRadialProgress = m_fRadialProgress;

	return data;
}

void ImageBox::ShowControllImGui()
{
	IUIComponent::ShowControllImGui();
	ImGui::InputFloat2("Size", (float*)&m_v2Size);
}

UIRectData ImageButton::MakeSBData() const
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

void ImageButton::ShowControllImGui()
{
	IUIComponent::ShowControllImGui();
	ImGui::InputFloat2("Size", (float*)&m_v2Size);
}
