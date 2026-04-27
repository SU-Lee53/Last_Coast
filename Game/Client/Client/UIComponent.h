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
	virtual bool IsClickable() const { return false; }

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

	virtual UIRectData MakeSBData() const = 0;
	virtual const TextureRef<Texture>& GetTextureRef() const { return {}; }

	RECT GetScreenRect() const;

protected:
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
	bool WasHovered() { return m_bHovered; }

	void SetButtonCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnClickCallback = fnCallback;
	}

	void SetBeginHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnBeginHoverCallback = fnCallback;
	}

	void SetEndHoverCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_fnEndHoverCallback = fnCallback;
	}

	bool CheckPointInComponent(POINT ptClicked) const {
		RECT r = GetScreenRect();
		return (ptClicked.x >= r.left && ptClicked.x <= r.right) &&
			(ptClicked.y >= r.top && ptClicked.y <= r.bottom);
	}

	void OnClicked() {
		//m_bHovered = false;
		if (m_fnClickCallback) m_fnClickCallback(this);
	}

	void OnBeginHovered() {
		m_bHovered = true;
		if (m_fnBeginHoverCallback) m_fnBeginHoverCallback(this);
	}

	void OnEndHovered() {
		m_bHovered = false;
		if (m_fnEndHoverCallback) m_fnEndHoverCallback(this);
	}

protected:
	// Button
	std::function<void(IUIComponent*)> m_fnClickCallback;
	std::function<void(IUIComponent*)> m_fnBeginHoverCallback;
	std::function<void(IUIComponent*)> m_fnEndHoverCallback;
	bool m_bHovered = false;

};

template<typename C>
concept Clickable = requires(C comp) {
	comp->IsClickable();
};
