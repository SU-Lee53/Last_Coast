#include "pch.h"
#include "UIComponent.h"

const TextureRef<Texture>& IUIComponent::GetTextureRef() const
{
	static const TextureRef<Texture> g_EmptyTextureRef{};
	return g_EmptyTextureRef;
}

RECT IUIComponent::GetScreenRect() const
{
	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);

	if (m_bFillScreen) {
		return RECT{
			0,
			0,
			static_cast<LONG>(WinCore::g_dwClientWidth),
			static_cast<LONG>(WinCore::g_dwClientHeight)
		};
	}

	const float fScale = std::min(
		fScreenWidth / g_fReferenceScreenWidth,
		fScreenHeight / g_fReferenceScreenHeight
	);

	const Vector2 v2CanvasSize = {
		g_fReferenceScreenWidth * fScale,
		g_fReferenceScreenHeight * fScale
	};

	const Vector2 v2CanvasOffset = {
		(fScreenWidth - v2CanvasSize.x) * 0.5f,
		(fScreenHeight - v2CanvasSize.y) * 0.5f
	};

	const Vector2 v2AnchorPos = {
		v2CanvasOffset.x + m_v2Anchor.x * v2CanvasSize.x,
		v2CanvasOffset.y + m_v2Anchor.y * v2CanvasSize.y
	};

	const Vector2 v2ScaledPosition = m_v2Position * fScale;
	const Vector2 v2ScaledSize = m_v2Size * fScale;
	const Vector2 v2BasePos = v2AnchorPos + v2ScaledPosition;

	const Vector2 v2LeftTop = {
		v2BasePos.x - m_v2Pivot.x * v2ScaledSize.x,
		v2BasePos.y - m_v2Pivot.y * v2ScaledSize.y
	};

	RECT r{};
	r.left = static_cast<LONG>(std::round(v2LeftTop.x));
	r.top = static_cast<LONG>(std::round(v2LeftTop.y));
	r.right = static_cast<LONG>(std::round(v2LeftTop.x + v2ScaledSize.x));
	r.bottom = static_cast<LONG>(std::round(v2LeftTop.y + v2ScaledSize.y));

	return r;
}

void IUIComponent::ShowControllImGui()
{
	ImGui::Text("Visible"); ImGui::SameLine();
	if (ImGui::Button(m_bVisible ? "ON" : "OFF")) {
		m_bVisible = !m_bVisible;
	}
	ImGui::InputFloat4("Color", (float*)&m_v4Color);
	ImGui::InputFloat2("Anchor", (float*)&m_v2Anchor);
	ImGui::InputFloat2("Pivot", (float*)&m_v2Pivot);
	ImGui::InputFloat2("Position", (float*)&m_v2Position);
}

Vector2 IUIComponent::ScreenToReferencePosition(const Vector2& v2ScreenPosition)
{
	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);
	const float fScale = std::min(
		fScreenWidth / g_fReferenceScreenWidth,
		fScreenHeight / g_fReferenceScreenHeight
	);

	if (fScale <= 0.f)
		return Vector2::Zero;

	const Vector2 v2CanvasSize = {
		g_fReferenceScreenWidth * fScale,
		g_fReferenceScreenHeight * fScale
	};

	const Vector2 v2CanvasOffset = {
		(fScreenWidth - v2CanvasSize.x) * 0.5f,
		(fScreenHeight - v2CanvasSize.y) * 0.5f
	};

	return Vector2{
		(v2ScreenPosition.x - v2CanvasOffset.x) / fScale,
		(v2ScreenPosition.y - v2CanvasOffset.y) / fScale
	};
}
