#pragma once
#include "UIComponent.h"

class IGameObject;

interface IText abstract {
public:
	const static std::wstring g_wstrDefaultFontName;

public:
	IText(Font::ID font);
	IText(const std::wstring& wstrFontname);

	void SetFontID(Font::ID fontID);
	virtual void SetText(const std::wstring& wstrText);
	virtual void SetText(const std::string& strText);

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
	TextBox(const std::wstring& wstrFontname = IText::g_wstrDefaultFontName) : IText{ wstrFontname } {}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

	virtual const std::wstring& GetName() const override { return m_wstrText; }
	virtual void ShowControllImGui() override;
};

class TextButton : public IUIButtonComponent, public IText {
public:
	TextButton(Font::ID font) 
		: IText{ font } {
		SetColor(Vector3{ 0.8, 0.8, 0.8 });
		m_fnBeginHoverCallback = g_fnDefaultBeginHover;
		m_fnEndHoverCallback = g_fnDefaultEndHover;
	}
	TextButton(const std::wstring& wstrFontname = IText::g_wstrDefaultFontName) : IText{ wstrFontname } {
		SetColor(Vector3{ 0.8, 0.8, 0.8 });
		m_fnBeginHoverCallback = g_fnDefaultBeginHover;
		m_fnEndHoverCallback = g_fnDefaultEndHover;
	}

	virtual void Update() override;
	virtual UIRectData MakeSBData() const override;

	virtual const std::wstring& GetName() const override { return m_wstrText; }
	virtual void ShowControllImGui() override;

	static void g_fnDefaultBeginHover(IUIComponent* pComp) {
		auto pButton = static_cast<TextBox*>(pComp);
		pButton->SetColor(Vector3(1.0, 0.0, 0.0));
	}

	static void g_fnDefaultEndHover(IUIComponent* pComp) {
		auto pButton = static_cast<TextBox*>(pComp);
		pButton->SetColor(Vector3{ 0.8, 0.8, 0.8 });
	}

};

class InputTextBox : public IUIFocusableComponent, public IText {
public:
	InputTextBox(Font::ID font)
		: IText{ font } {
		SetText(m_wstrCommittedText);
		m_wstrCommittedText.reserve(m_maxLength);
		m_wstrCompositionText.reserve(m_maxLength);
	}

	InputTextBox(const std::wstring& wstrFontname = IText::g_wstrDefaultFontName)
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
	void ClearText() { m_wstrCommittedText.clear(); RefreshDisplayText(); }	// 입력 내용 비우기

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

class TextBillboard : public TextBox {
public:
	TextBillboard(Font::ID font) : TextBox{ font } {}
	TextBillboard(const std::wstring& wstrFontname) : TextBox{ wstrFontname } {}

	virtual void Update() override;

	void SetTarget(const std::shared_ptr<IGameObject>& pTarget) { m_wpTarget = pTarget; }
	void SetWorldOffset(const Vector3& v3WorldOffset) { m_v3WorldOffset = v3WorldOffset; }
	void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }
	void SetMaxDistance(float fMaxDistance) { m_fMaxDistance = fMaxDistance; }
	void SetDistanceScale(float fNearDistance, float fFarDistance, float fNearScale, float fFarScale);

private:
	std::weak_ptr<IGameObject> m_wpTarget;
	Vector3 m_v3WorldOffset{};
	float m_fMaxDistance = 0.f;
	float m_fNearScaleDistance = 0.f;
	float m_fFarScaleDistance = 0.f;
	float m_fNearScale = 1.f;
	float m_fFarScale = 1.f;
	bool m_bEnabled = true;
};
