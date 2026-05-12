#include "pch.h"
#include "TextBox.h"

//////////////////////////////////////////////////////////////////////////////////////////
// IText

IText::IText(Font::ID font)
{
	m_fontID = font;
	m_bDirty = true;
}

IText::IText(const std::wstring& wstrFontname)
{
	m_fontID = RENDER->GetTextRenderer().GetFontID(wstrFontname);
	m_bDirty = true;
}

void IText::SetFontID(Font::ID fontID)
{
	if (m_fontID == fontID) return;

	m_fontID = fontID;
	m_bDirty = true;
}

void IText::SetText(const std::wstring& wstrText)
{
	if (m_wstrText == wstrText) return;

	m_wstrText = wstrText;
	m_bDirty = true;
}

Vector4 IText::GetTextUV() const
{
	auto pCached = m_TextHandle.GetResource();
	const auto& rect = pCached->Rect;

	float u0 = static_cast<float>(rect.x) / TextRenderer::g_unAtlasWidth;
	float v0 = static_cast<float>(rect.y) / TextRenderer::g_unAtlasHeight;

	float u1 = static_cast<float>(rect.x + rect.w) / TextRenderer::g_unAtlasWidth;
	float v1 = static_cast<float>(rect.y + rect.h) / TextRenderer::g_unAtlasHeight;

	return { u0,v0,u1,v1 };
}

void IText::RefreshTextHandle()
{
	if (!m_bDirty || m_fontID == INVALID_ID) {
		return;
	}

	m_TextHandle = RENDER->GetTextRenderer().GetOrCacheText(m_fontID, m_wstrText);
	m_bDirty = false;

}

//////////////////////////////////////////////////////////////////////////////////////////
// TextBox

void TextBox::Update()
{
	m_v2Size = Vector2(m_v2SizePerLetter.x * m_wstrText.length(), m_v2SizePerLetter.y);
	RefreshTextHandle();
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

void TextButton::Update()
{
	RefreshTextHandle();
}

UIRectData TextButton::MakeSBData() const
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

