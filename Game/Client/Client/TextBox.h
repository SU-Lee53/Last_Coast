#pragma once
#include "UIComponent.h"

interface IText abstract {
public:
	IText(Font::ID font);
	IText(const std::wstring& wstrFontname);

	void SetFontID(Font::ID fontID);
	virtual void SetText(const std::wstring& wstrText);

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

	void SetSize(const Vector2& v2Size) = delete;
	void SetSizePerLetter(const Vector2& v2Size) { m_v2SizePerLetter = v2Size; };

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

private:
	//size_t m_unMaxLength = 0;
	Vector2 m_v2SizePerLetter = Vector2::Zero;
};

class TextButton : public IUIButtonComponent, public IText {
public:
	TextButton(Font::ID font) : IText{ font } {}
	TextButton(const std::wstring& wstrFontname) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;
};
