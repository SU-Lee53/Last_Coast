#pragma once
#include "UIComponent.h"

class TextBox : public IUIComponent {

public:
	TextBox(Font::ID font);
	TextBox(const std::wstring& wstrFontname);

	void SetFontID(Font::ID fontID);
	void SetText(const std::wstring& wstrText);
	void SetTextColor(const Vector3& v3Color) { m_v4Color = Vector4(v3Color.x, v3Color.y, v3Color.z, 1.f); }

	const std::wstring& GetText() const { return m_wstrText; }
	Font::ID GetFontID() const { return m_fontID; }

	virtual void Update() override;

	virtual UIRectData MakeSBData() const override;

protected:
	void RefreshTextHandle();
	Vector4 GetTextUV() const;

protected:
	Font::ID m_fontID = INVALID_ID;
	std::wstring m_wstrText{};
	TextHandle m_TextHandle{};

	bool m_bDirty = true;

};
