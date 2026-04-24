#pragma once
#include "UIComponent.h"

class TextBox : public IUIComponent {

public:
	TextBox(Font::ID font);
	TextBox(const std::wstring& wstrFontname);

	void SetFontID(Font::ID fontID);
	void SetText(const std::wstring& wstrText);
	void SetTextColor(const Vector4& v4Color) { m_v4TextColor = v4Color; }

	const std::wstring& GetText() const { return m_wstrText; }
	Font::ID GetFontID() const { return m_fontID; }
	const Vector4& GetTextColor() { return m_v4TextColor; }

	virtual void Update() override;

	virtual UIRectData MakeSBData() const override;

protected:
	void RefreshTextHandle();
	Vector4 GetTextUV() const;

protected:
	Vector4 m_v4TextColor = Vector4{1.f, 1.f, 1.f, 1.f};

	Font::ID m_fontID = INVALID_ID;
	std::wstring m_wstrText{};
	TextHandle m_TextHandle{};

	bool m_bDirty = true;

};
