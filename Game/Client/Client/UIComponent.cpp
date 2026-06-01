#include "pch.h"
#include "UIComponent.h"

RECT IUIComponent::GetScreenRect() const
{
	const float fScreenWidth = static_cast<float>(WinCore::g_dwClientWidth);
	const float fScreenHeight = static_cast<float>(WinCore::g_dwClientHeight);

	// 1. 화면 기준 Anchor 위치 계산
	const Vector2 v2AnchorPos = {
		m_v2Anchor.x * fScreenWidth,
		m_v2Anchor.y * fScreenHeight
	};

	// 2. Anchor 기준 픽셀 오프셋 적용
	const Vector2 v2BasePos = {
		v2AnchorPos.x + m_v2Position.x,
		v2AnchorPos.y + m_v2Position.y
	};

	// 3. Pivot 을 반영한 최종 좌상단 계산
	const Vector2 v2LeftTop = {
		v2BasePos.x - m_v2Pivot.x * m_v2Size.x,
		v2BasePos.y - m_v2Pivot.y * m_v2Size.y,
	};

	RECT r{};
	r.left = static_cast<LONG>(std::round(v2LeftTop.x));
	r.top = static_cast<LONG>(std::round(v2LeftTop.y));
	r.right = static_cast<LONG>(std::round(v2LeftTop.x + m_v2Size.x));
	r.bottom = static_cast<LONG>(std::round(v2LeftTop.y + m_v2Size.y));

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
