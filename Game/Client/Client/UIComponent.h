#pragma once

struct UIRectData
{
	Vector4 v4ScreenRect;
	Vector4 v4UVRect;
	Vector4 v4TextColorOrTexIndex;	// Text -> float4 color / Sprite -> float4.x texIndex
};

interface IUIComponent abstract {
public:
	virtual ~IUIComponent() = default;

	virtual void Update() {};
	bool CheckClicked(POINT ptClicked);
	void OnClicked() { if (m_fnCallback) m_fnCallback(this); }

public:
	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	void SetLayer(uint32 unLayer) { m_unLayer = unLayer; }
	void SetAnchor(const Vector2& v2Anchor) { m_v2Anchor = v2Anchor; }
	void SetPivot(const Vector2& v2Pivot) { m_v2Pivot = v2Pivot; }
	void SetPosition(const Vector2& v2Pos) { m_v2Position = v2Pos; }
	void SetSize(const Vector2& v2Size) { m_v2Size = v2Size; }
	
	void SetButtonCallback(std::function<void(IUIComponent*)> fnCallback) {
		m_bClickable = true;
		m_fnCallback = fnCallback;
	}

	bool IsVisible() const { return m_bVisible; }
	bool IsClickable() const { return m_bClickable && m_bVisible; }
	uint32 GetLayer() const { return m_unLayer; }
	const Vector2& GetAnchor() const { return m_v2Anchor; }
	const Vector2& GetPivot() const { return m_v2Pivot; }
	const Vector2& GetPosition() const { return m_v2Position; }
	const Vector2& GetSize() const { return m_v2Size; }

	virtual UIRectData MakeSBData() const = 0;

	RECT GetScreenRect() const;

protected:
	uint32 m_unLayer = 0;

	Vector2 m_v2Anchor = Vector2{0.f, 0.f};	// 0 ~ 1 정규화 기준점
	Vector2 m_v2Pivot = Vector2{0.f, 0.f};	// 박스 내부 기준점

	Vector2 m_v2Position = Vector2{ 0.f, 0.f };	// anchor 기준 픽셀 오프셋
	Vector2 m_v2Size = Vector2{ 0.f, 0.f };		// 픽셀 크기

	bool m_bVisible = true;

	// Button
	bool m_bClickable = false;
	std::function<void(IUIComponent*)> m_fnCallback;

};

