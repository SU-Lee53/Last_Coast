#include "pch.h"
#include "UIBoard.h"
#include "TextBox.h"
#include "Sprite.h"

void UIBoard::InsertUI(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_UILayers[unLayer].push_back(pComponent);
}

void UIBoard::RemoveUI(const std::shared_ptr<IUIComponent>& pComponent)
{
	uint32 unLayer = pComponent->GetLayer();
	m_UILayers[unLayer].erase(std::remove(m_UILayers[unLayer].begin(), m_UILayers[unLayer].end(), pComponent), m_UILayers[unLayer].end());
}

void UIBoard::Update()
{
	bool bClicked = INPUT->GetButtonDown(VK_LBUTTON);
	POINT ptCursorPos = INPUT->GetCurrentCursorPos();
	bool bFocusTargetFound = false;

	for (auto& layer : m_UILayers) {
		for (auto& pComp : layer) {
			if (pComp->IsClickable()) {
				//auto& pButton = static_pointer_cast<IUIButtonComponent>(pComp);
				bool bCursorOnButton = pComp->CheckPointInComponent(ptCursorPos);

				if (pComp->IsClickable()) {
					if (!bCursorOnButton) {
						if (pComp->WasHovered()) {
							pComp->OnEndHovered();
						}

						pComp->Update();
						continue;
					}

					if (bClicked) {
						pComp->OnClicked();
					}
					else {
						if (!pComp->WasHovered()) {
							pComp->OnBeginHovered();
						}
					}
				}

				if (bClicked && bCursorOnButton && pComp->IsFocusable()) {
					SetFocus(pComp);
					bFocusTargetFound = true;
				}

			}

			pComp->Update();
		}
	}

	if (bClicked && !bFocusTargetFound) {
		ClearFocus();
	}
}

void UIBoard::SetFocus(const std::shared_ptr<IUIComponent>& pFocus)
{
	if (auto pPrev = m_pCurrentFocused.lock()) {
		if (pPrev == pFocus)
			return;

		pPrev->SetFocused(false);
	}

	m_pCurrentFocused = pFocus;

	if (pFocus) {
		pFocus->SetFocused(true);
	}
}

void UIBoard::ClearFocus()
{
	if (auto pPrev = m_pCurrentFocused.lock()) {
		pPrev->SetFocused(false);
	}

	m_pCurrentFocused.reset();
}

void UIBoard::OnChar(wchar_t ch)
{
	if (auto pFocused = m_pCurrentFocused.lock()) {
		pFocused->OnChar(ch);
	}
}

void UIBoard::EditUI()
{
	if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { 
		m_nSelectedLayer--;
		m_nSelectedLayer %= g_unUILayers;
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { 
		m_nSelectedLayer++;
		m_nSelectedLayer %= g_unUILayers;
	}
	ImGui::SameLine();
	ImGui::Text("%d", m_nSelectedLayer);

	if (ImGui::BeginListBox("UIComponents in Layer")) {
		for (int32 i = 0; i < m_UILayers[m_nSelectedLayer].size(); ++i) {
			bool bSelected = (m_nSelectedUIIndex == i);
			auto& curComponent = m_UILayers[m_nSelectedLayer][i];
			if (ImGui::Selectable(WStringToString(curComponent->GetName()).c_str(), bSelected)) {
				m_nSelectedUIIndex = i;
			}

			if (bSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndListBox();
	}

	ImGui::SeparatorText("Current UIComponent Modifier");
	auto& selectedComponent = m_UILayers[m_nSelectedLayer][m_nSelectedUIIndex];
	selectedComponent->ShowControllImGui();
	if (ImGui::Button("Delete this")) {
		auto& curLayer = m_UILayers[m_nSelectedLayer];
		std::erase(curLayer, selectedComponent);
	}

	/*
		m_pTimeText = std::make_shared<TextBox>(L"Malgun Gothic");
		m_pTimeText->SetText(L"Test");
		m_pTimeText->SetLayer(0);
		m_pTimeText->SetAnchor(Vector2{ 0,0 });
		m_pTimeText->SetPivot(Vector2{ 0,0 });
		m_pTimeText->SetPosition(Vector2{ 200,50 });
		m_pTimeText->SetTextHeight(50);
	*/

	ImGui::SeparatorText("Add New");
	ImGui::InputText("Image path or Text", &m_strNewName);
	ImGui::InputInt("Layer", &m_nNewLayer);
	ImGui::DragFloat2("Anchor", (float*)&m_v2NewAnchor, 0.1f, 0.f, 1600.f);
	ImGui::DragFloat2("Pivot", (float*)&m_v2NewPivot, 0.1f, 0.f, 1600.f);
	ImGui::DragFloat2("Position", (float*)&m_v2NewPosition, 0.1f, 0.f, 1600.f);
	ImGui::DragFloat("Width", &m_fNewWidth);
	ImGui::DragFloat("Height (or Text height)", &m_fNewHeight);

	if (ImGui::Button("Add ImageSprite")) {

	}
	ImGui::SameLine();

	if (ImGui::Button("Add ImageButton")) {

	}

	if (ImGui::Button("Add TextBox")) {
		std::shared_ptr<TextBox> pNew = std::make_shared<TextBox>(L"Malgun Gothic");
		pNew->SetText(StringToWString(m_strNewName));
		pNew->SetLayer(m_nNewLayer);
		pNew->SetAnchor(m_v2NewAnchor);
		pNew->SetPivot(m_v2NewPivot);
		pNew->SetPosition(m_v2NewPosition);
		pNew->SetTextHeight(m_fNewHeight);

		m_UILayers[m_nNewLayer].push_back(pNew);
	}
	ImGui::SameLine();

	if (ImGui::Button("Add TextButton")) {

		std::shared_ptr<TextButton> pNew = std::make_shared<TextButton>(L"Malgun Gothic");
		pNew->SetText(StringToWString(m_strNewName));
		pNew->SetLayer(m_nNewLayer);
		pNew->SetAnchor(m_v2NewAnchor);
		pNew->SetPivot(m_v2NewPivot);
		pNew->SetPosition(m_v2NewPosition);
		pNew->SetTextHeight(m_fNewHeight);

		m_UILayers[m_nNewLayer].push_back(pNew);
	}
	ImGui::SameLine();

	if (ImGui::Button("Add InputText")) {
		std::shared_ptr<InputTextBox> pNew = std::make_shared<InputTextBox>(L"Malgun Gothic");
		pNew->SetPlaceholder(StringToWString(m_strNewName));
		pNew->SetLayer(m_nNewLayer);
		pNew->SetAnchor(m_v2NewAnchor);
		pNew->SetPivot(m_v2NewPivot);
		pNew->SetPosition(m_v2NewPosition);
		pNew->SetTextHeight(m_fNewHeight);

		m_UILayers[m_nNewLayer].push_back(pNew);
	}
	ImGui::SameLine();


}

void UIBoard::SaveUI(const std::string& strFilename)
{
}

void UIBoard::LoadUI(const std::string& strFilename)
{
}
