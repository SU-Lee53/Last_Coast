#pragma once

struct UIRectData
{
	Vector4 v4ScreenRect;
	Vector4 v4UVRect;
	Vector4 v4Color;
	int32	nTexIndex;
};

interface IUIComponent abstract {
public:
	virtual ~IUIComponent() = default;

	virtual void Update() {};

public:
	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	void SetLayer(uint32 unLayer) { m_unLayer = unLayer; }
	void SetAnchor(const Vector2& v2Anchor) { m_v2Anchor = v2Anchor; }
	void SetPivot(const Vector2& v2Pivot) { m_v2Pivot = v2Pivot; }
	void SetPosition(const Vector2& v2Pos) { m_v2Position = v2Pos; }
	void SetSize(const Vector2& v2Size) { m_v2Size = v2Size; }
	void SetColor(const Vector4& v4Color) { m_v4Color = v4Color; }
	void SetColor(const Vector3& v3Color) { m_v4Color = Vector4(v3Color.x, v3Color.y, v3Color.z, 1.f); }

	bool IsVisible() const { return m_bVisible; }
	uint32 GetLayer() const { return m_unLayer; }
	const Vector2& GetAnchor() const { return m_v2Anchor; }
	const Vector2& GetPivot() const { return m_v2Pivot; }
	const Vector2& GetPosition() const { return m_v2Position; }
	const Vector2& GetSize() const { return m_v2Size; }
	const Vector4& GetColor() { return m_v4Color; }

public:
	virtual bool IsClickable() const { return false; }
	virtual bool IsFocusable() const { return false; }

	virtual void SetFocused(bool bFocused) {}
	virtual bool IsFocused() const { return false; }

	virtual bool WasHovered() const { return false; }
	virtual void OnClicked() {}
	virtual void OnBeginHovered() {}
	virtual void OnEndHovered() {}

	virtual void OnChar(wchar_t) {}

	bool CheckPointInComponent(POINT ptClicked) const {
		RECT r = GetScreenRect();
		return (ptClicked.x >= r.left && ptClicked.x <= r.right) &&
			(ptClicked.y >= r.top && ptClicked.y <= r.bottom);
	}

public:
	virtual UIRectData MakeSBData() const = 0;
	virtual const TextureRef<Texture>& GetTextureRef() const { return {}; }

	RECT GetScreenRect() const;
	virtual const std::wstring& GetName() const { return m_strName; }

	virtual void ShowControllImGui();


protected:
	std::wstring m_strName;

	uint32 m_unLayer = 0;
	Vector4 m_v4Color = Vector4{ 1.f, 1.f, 1.f, 1.f };

	Vector2 m_v2Anchor = Vector2{0.f, 0.f};	// 0 ~ 1 정규화 기준점
	Vector2 m_v2Pivot = Vector2{0.f, 0.f};	// 박스 내부 기준점

	Vector2 m_v2Position = Vector2{ 0.f, 0.f };	// anchor 기준 픽셀 오프셋
	Vector2 m_v2Size = Vector2{ 0.f, 0.f };		// 픽셀 크기

	bool m_bVisible = true;

};

interface IUIButtonComponent abstract : public IUIComponent {
public:
	virtual bool IsClickable() const override { return m_bVisible; }
	virtual bool WasHovered() const override { return m_bHovered; }

	virtual void OnClicked() override {
		//m_bHovered = false;
		if (m_fnClickCallback) m_fnClickCallback(this);
	}

	virtual void OnBeginHovered() override {
		m_bHovered = true;
		if (m_fnBeginHoverCallback) m_fnBeginHoverCallback(this);
	}

	virtual void OnEndHovered() override {
		m_bHovered = false;
		if (m_fnEndHoverCallback) m_fnEndHoverCallback(this);
	}

public:
	void SetButtonCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnClickCallback = fnCallback;
	}

	void SetBeginHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnBeginHoverCallback = fnCallback;
	}

	void SetEndHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnEndHoverCallback = fnCallback;
	}

protected:
	// Button
	std::function<void(IUIComponent*)> m_fnClickCallback;
	std::function<void(IUIComponent*)> m_fnBeginHoverCallback;
	std::function<void(IUIComponent*)> m_fnEndHoverCallback;
	bool m_bHovered = false;

};

interface IUIFocusableComponent abstract : public IUIComponent{
public:
	virtual bool IsClickable() const override { return m_bVisible; }
	virtual bool IsFocusable() const { return m_bVisible; }
	virtual bool IsFocused() const override { return m_bFocused; }

	virtual void SetFocused(bool bFocused) override {
		if (m_bFocused == bFocused) {
			return;
		}

		m_bFocused = bFocused;

		if (m_bFocused) {
			if (m_fnBeginFocusCallback) m_fnBeginFocusCallback(this);
		}
		else {
			if (m_fnEndFocusCallback) m_fnEndFocusCallback(this);
		}
	}

public:
	void SetButtonCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnClickCallback = fnCallback;
	}

	void SetBeginHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnBeginHoverCallback = fnCallback;
	}

	void SetEndHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnEndHoverCallback = fnCallback;
	}

	void SetBeginFocusCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnBeginFocusCallback = fnCallback;
	}

	void SetEndFocusCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnEndFocusCallback = fnCallback;
	}

	void SetEnterCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnEnterCallback = fnCallback;
	}

protected:
	std::function<void(IUIComponent*)> m_fnClickCallback;
	std::function<void(IUIComponent*)> m_fnBeginHoverCallback;
	std::function<void(IUIComponent*)> m_fnEndHoverCallback;
	std::function<void(IUIComponent*)> m_fnBeginFocusCallback;
	std::function<void(IUIComponent*)> m_fnEndFocusCallback;
	std::function<void(IUIComponent*)> m_fnEnterCallback;

	bool m_bFocused = false;
};

template<typename C>
concept Clickable = requires(C comp) {
	comp->IsClickable();
};

template<typename C>
concept Focusable = requires(C comp) {
	comp->IsFocusable();
};
