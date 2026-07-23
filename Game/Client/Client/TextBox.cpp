#include "pch.h"
#include "TextBox.h"

//////////////////////////////////////////////////////////////////////////////////////////
// IText

const std::wstring IText::g_wstrDefaultFontName = L"Noto Sans KR";

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

void IText::SetText(const std::string& strText)
{
	return SetText(StringToWString(strText));
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

void IText::SetTextHeight(float fHeight)
{
	m_fTargetTextHeight = fHeight;
	m_bUseTargetTextHeight = true;
}

Vector2 IText::GetCachedTextSize() const
{
	if (!m_TextHandle.IsValid()) {
		return {};
	}

	auto pCached = m_TextHandle.GetResource();

	if (!pCached || !pCached->bValid)
		return Vector2::Zero;

	return pCached->GetSize();
}

Vector2 IText::CalculateScaledTextSize() const
{
	if (!m_TextHandle.IsValid()) {
		return Vector2::Zero;
	}

	auto pCached = m_TextHandle.GetResource();

	Vector2 cachedSize{
		static_cast<float>(pCached->Rect.w),
		static_cast<float>(pCached->Rect.h)
	};

	if (!m_bUseTargetTextHeight || cachedSize.y <= 0.f)
		return cachedSize;

	float scale = m_fTargetTextHeight / cachedSize.y;

	return Vector2{
		cachedSize.x * scale,
		cachedSize.y * scale
	};
}

//////////////////////////////////////////////////////////////////////////////////////////
// TextBox

void TextBox::Update()
{
	RefreshTextHandle();

	if (m_bUseTargetTextHeight) {
		Vector2 textSize = CalculateScaledTextSize();

		if (textSize.x <= 0.f)
			textSize.x = 1.f;

		if (textSize.y <= 0.f)
			textSize.y = m_fTargetTextHeight;

		m_v2Size = textSize;
	}
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

void TextBox::ShowControllImGui()
{
	std::string strInput = WStringToString(m_wstrText);
	if (ImGui::InputText("Text", &strInput)) {
		SetText(StringToWString(strInput));
	}
	IUIComponent::ShowControllImGui();
}

void TextButton::Update()
{
	RefreshTextHandle();

	if (m_bUseTargetTextHeight) {
		Vector2 textSize = CalculateScaledTextSize();

		if (textSize.x <= 0.f)
			textSize.x = 1.f;

		if (textSize.y <= 0.f)
			textSize.y = m_fTargetTextHeight;

		m_v2Size = textSize;
	}
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

void TextButton::ShowControllImGui()
{
	std::string strInput = WStringToString(m_wstrText);
	if (ImGui::InputText("Text", &strInput)) {
		SetText(StringToWString(strInput));
	}
	IUIComponent::ShowControllImGui();
}

void InputTextBox::Update()
{
	if (IsFocused()) {
		m_fCaretTimer += DT;

		if (m_fCaretTimer >= 0.5f) {
			m_fCaretTimer = 0.f;
			m_bCaretVisible = !m_bCaretVisible;
			RefreshDisplayText();
		}
	}
	else {
		m_fCaretTimer = 0.f;
		m_bCaretVisible = false;

		RefreshDisplayText();
	}

	RefreshTextHandle();

	Vector2 textSize = CalculateScaledTextSize();

	if (textSize.x <= 0.f)
		textSize.x = 1.f;

	if (textSize.y <= 0.f)
		textSize.y = m_fTargetTextHeight;

	m_v2Size = textSize;
}

UIRectData InputTextBox::MakeSBData() const
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

void InputTextBox::SetFocused(bool bFocused)
{
	IUIFocusableComponent::SetFocused(bFocused);

	m_fCaretTimer = 0.f;
	m_bCaretVisible = true;

	RefreshDisplayText();
}

void InputTextBox::SetPlaceholder(const std::wstring& wstrPlaceholder)
{
	m_wstrPlaceholderText = wstrPlaceholder;
	RefreshDisplayText();
}

void InputTextBox::SetPasswordMode(bool bEnable)
{
	m_bPasswordMode = bEnable;
	RefreshDisplayText();
}

void InputTextBox::OnChar(wchar_t ch)
{
	if (!IsFocused())
		return;

	// Backspace
	if (ch == L'\b') {
		if (!m_wstrCommittedText.empty()) {
			m_wstrCommittedText.pop_back();
			RefreshDisplayText();
		}
		return;
	}

	// Enter
	if (ch == L'\r') {
		if (m_fnEnterCallback) {
			m_fnEnterCallback(static_cast<IUIComponent*>(this));
		}
		return;
	}

	// ignore control character
	if (ch < 32) {
		return;
	}

	// limits maximum length
	if (m_wstrCommittedText.length() >= m_maxLength) {
		return;
	}

	m_wstrCommittedText.push_back(ch);
	RefreshDisplayText();
}

void InputTextBox::RefreshDisplayText()
{
	const bool bShowPlaceholder =
		!IsFocused() &&
		m_wstrCommittedText.empty() &&
		!m_wstrPlaceholderText.empty();

	if (bShowPlaceholder) {
		SetText(m_wstrPlaceholderText);
		m_v4Color = m_v4PlaceholderColor;
		return;
	}

	std::wstring wstrDisplayText = m_wstrCommittedText + m_wstrCompositionText;
	if (m_bPasswordMode) {
		wstrDisplayText.assign(m_wstrCommittedText.length(), m_chPasswordMask);
	}
	else {
		wstrDisplayText = m_wstrCommittedText;
	}

	wstrDisplayText += m_wstrCompositionText;

	if (IsFocused() && m_bCaretVisible) {
		wstrDisplayText += L"|";
	}

	SetText(wstrDisplayText);
	m_v4Color = m_v4TextColor;
}

void InputTextBox::ShowControllImGui()
{
	std::string strInput = WStringToString(m_wstrPlaceholderText);
	if (ImGui::InputText("PlaceHolder", &strInput)) {
		m_wstrPlaceholderText = StringToWString(strInput);
	}
	IUIComponent::ShowControllImGui();
}

void TextBillboard::Update()
{
	TextBox::Update();

	std::shared_ptr<IGameObject> pTarget = m_wpTarget.lock();
	const std::shared_ptr<Camera>& pCamera = CUR_SCENE->GetCamera();

	// Make invisible when...
	//	- UI Disabled
	//	- Target or camera is null
	//	- Character is hiding
	if (!m_bEnabled || !pTarget || !pCamera || (pTarget->IsCharacter() && CUR_SCENE->IsHidingCharacters())) {
		SetVisible(false);
		return;
	}

	Vector3 v3WorldPosition = pTarget->GetTransform()->GetPosition() + m_v3WorldOffset;

	// Distance cutoff
	float fDistance = Vector3::Distance(v3WorldPosition, pCamera->GetPosition());
	if (m_fMaxDistance > 0.f && fDistance > m_fMaxDistance) {
		SetVisible(false);
		return;
	}

	// VP Transform target position
	Vector4 v4ClipPosition = Vector4::Transform(
		Vector4{ v3WorldPosition.x, v3WorldPosition.y, v3WorldPosition.z, 1.0f },
		pCamera->GetViewProjectMatrix()
	);

	// Make invisivle when position is...
	//	- behind camera
	//	- out of depth range
	if (v4ClipPosition.w <= 0.01f ||
		v4ClipPosition.z < 0.0f ||
		v4ClipPosition.z > v4ClipPosition.w) {
		SetVisible(false);
		return;
	}

	// Make invisivle when position is out of screen space
	float fNdcX = v4ClipPosition.x / v4ClipPosition.w;
	float fNdcY = v4ClipPosition.y / v4ClipPosition.w;

	if (fNdcX < -1.0f || fNdcX > 1.0f ||
		fNdcY < -1.0f || fNdcY > 1.0f) {
		SetVisible(false);
		return;
	}

	// Transform into screen space
	float fScreenX = (fNdcX + 1.0f) * 0.5f * static_cast<float>(WinCore::g_dwClientWidth);
	float fScreenY = (1.0f - fNdcY) * 0.5f * static_cast<float>(WinCore::g_dwClientHeight);

	if (m_fFarScaleDistance > m_fNearScaleDistance) {
		float fScaleRatio = ::SmoothStep(fDistance, m_fNearScaleDistance, m_fFarScaleDistance);
		float fDistanceScale = std::lerp(m_fNearScale, m_fFarScale, fScaleRatio);
		m_v2Size.x *= fDistanceScale;
		m_v2Size.y *= fDistanceScale;
	}

	SetPosition(ScreenToReferencePosition(Vector2{ fScreenX, fScreenY }));
	SetVisible(true);
}

void TextBillboard::SetDistanceScale(float fNearDistance, float fFarDistance, float fNearScale, float fFarScale)
{
	if (fNearDistance < 0.f ||
		fFarDistance <= fNearDistance ||
		fNearScale <= 0.f ||
		fFarScale <= 0.f)
		return;

	m_fNearScaleDistance = fNearDistance;
	m_fFarScaleDistance = fFarDistance;
	m_fNearScale = fNearScale;
	m_fFarScale = fFarScale;
}
