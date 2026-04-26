#include "pch.h"
#include "TextBox.h"

TextBox::TextBox(Font::ID font)
{
	m_v4Color = Vector4(1.f, 1.f, 1.f, 1.f);
	m_fontID = font;
	m_bDirty = true;
}

TextBox::TextBox(const std::wstring& wstrFontname)
{
	m_fontID = RENDER->GetTextRenderer().GetFontID(wstrFontname);
	m_bDirty = true;
}

void TextBox::SetFontID(Font::ID fontID)
{
	if (m_fontID == fontID) return;

	m_fontID = fontID;
	m_bDirty = true;
}

void TextBox::SetText(const std::wstring& wstrText)
{
	if (m_wstrText == wstrText) return;

	m_wstrText = wstrText;
	m_bDirty = true;
}

void TextBox::Update()
{
	RefreshTextHandle();
}

Vector4 TextBox::GetTextUV() const
{
	auto pCached = m_TextHandle.GetResource();
	const auto& rect = pCached->Rect;

	float u0 = static_cast<float>(rect.x) / TextRenderer::g_unAtlasWidth;
	float v0 = static_cast<float>(rect.y) / TextRenderer::g_unAtlasHeight;

	float u1 = static_cast<float>(rect.x + rect.w) / TextRenderer::g_unAtlasWidth;
	float v1 = static_cast<float>(rect.y + rect.h) / TextRenderer::g_unAtlasHeight;

	return { u0,v0,u1,v1 };
}

UIRectData TextBox::MakeSBData() const
{
	RECT screenRect = GetScreenRect();
	RECT uvRect = {};

	UIRectData data;
	data.v4ScreenRect.x = static_cast<float>(screenRect.left);
	data.v4ScreenRect.y = static_cast<float>(screenRect.top);
	data.v4ScreenRect.z = static_cast<float>(screenRect.right);
	data.v4ScreenRect.w = static_cast<float>(screenRect.bottom);

	auto pCached = m_TextHandle.GetResource();
	if (!pCached || pCached->bDirty == true || pCached->bValid == false) {
		data.v4UVRect = Vector4{ 0,0,0,0 };
		data.v4Color = m_v4Color;
		return data;
	}

	data.v4UVRect = GetTextUV();
	data.v4Color = m_v4Color;

	data.nTexIndex = 0;

	return data;
}

void TextBox::RefreshTextHandle()
{
	if (!m_bDirty || m_fontID == INVALID_ID) {
		return;
	}

	m_TextHandle = RENDER->GetTextRenderer().GetOrCacheText(m_fontID, m_wstrText);
	m_bDirty = false;

}
