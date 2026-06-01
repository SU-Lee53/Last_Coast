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

	void SetTextHeight(float fHeight);
	Vector2 GetCachedTextSize() const;

protected:
	Vector2 CalculateScaledTextSize() const;

protected:
	void RefreshTextHandle();
	Vector4 GetTextUV() const;

protected:
	Font::ID m_fontID = INVALID_ID;
	std::wstring m_wstrText{};
	TextHandle m_TextHandle{};

	float m_fTargetTextHeight = 24.f;
	bool m_bUseTargetTextHeight = false;

	bool m_bDirty = true;
};

class TextBox : public IUIComponent, public IText {
public:
	TextBox(Font::ID font) : IText{ font } {}
	TextBox(const std::wstring& wstrFontname) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

	virtual const std::wstring& GetName() const override { return m_wstrText; }
	virtual void ShowControllImGui() override;
};

class TextButton : public IUIButtonComponent, public IText {
public:
	TextButton(Font::ID font) : IText{ font } {}
	TextButton(const std::wstring& wstrFontname) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

	virtual const std::wstring& GetName() const override { return m_wstrText; }
	virtual void ShowControllImGui() override;
};

class InputTextBox : public IUIFocusableComponent, public IText {
public:
	InputTextBox(Font::ID font)
		: IText{ font } {
		SetText(m_wstrCommittedText);
		m_wstrCommittedText.reserve(m_maxLength);
		m_wstrCompositionText.reserve(m_maxLength);
	}

	InputTextBox(const std::wstring& wstrFontname)
		: IText{ wstrFontname } {
		SetText(m_wstrCommittedText);
	}

	virtual void SetFocused(bool bFocused) override;
	void SetPlaceholder(const std::wstring& wstrPlaceholder);
	void SetPasswordMode(bool bEnable);
	virtual void OnChar(wchar_t ch) override;

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

	const std::wstring& GetCommittedText() const { return m_wstrCommittedText; }

	virtual const std::wstring& GetName() const override { return m_wstrCommittedText; }
	virtual void ShowControllImGui() override;

private:
	void RefreshDisplayText();

private:
	std::wstring m_wstrPlaceholderText = L"Placeholder";
	std::wstring m_wstrCommittedText;
	std::wstring m_wstrCompositionText;

private:
	size_t m_maxLength = 32;

	float m_fCaretTimer = 0.f;
	bool m_bCaretVisible = true;
	bool m_bFirstFocus = false;

	bool m_bPasswordMode = false;
	wchar_t m_chPasswordMask = L'*';

	Vector4 m_v4TextColor = Vector4{ 1.f, 1.f, 1.f, 1.f };
	Vector4 m_v4PlaceholderColor = Vector4{ 0.5f, 0.5f, 0.5f, 1.f };
};
