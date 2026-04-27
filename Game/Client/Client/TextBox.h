#pragma once
#include "UIComponent.h"

interface IText abstract {
public:
	IText(Font::ID font);
	IText(const std::wstring& wstrFontname);

	void SetFontID(Font::ID fontID);
	void SetText(const std::wstring& wstrText);

	const std::wstring& GetText() const { return m_wstrText; }
	Font::ID GetFontID() const { return m_fontID; }

protected:
	void RefreshTextHandle();
	Vector4 GetTextUV() const;

protected:
	Font::ID m_fontID = INVALID_ID;
	std::wstring m_wstrText{};
	TextHandle m_TextHandle{};

	bool m_bDirty = true;
};

class TextBox : public IUIComponent, public IText {
public:
	TextBox(Font::ID font) : IText{ font } {}
	TextBox(const std::wstring& wstrFontname) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;
};

class TextButton : public IUIButtonComponent, public IText {
public:
	TextButton(Font::ID font) : IText{ font } {}
	TextButton(const std::wstring& wstrFontname) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;
};
