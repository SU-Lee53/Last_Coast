#pragma once
#include "Scene.h"
#include "../../../Server/Server/protocol.h"

class InputTextBox;
class TextButton;
class TextBox;
class RoomListScene : public Scene {
public:
	virtual void BuildObjects() override;
	virtual void OnEnterScene() override;
	virtual void OnLeaveScene() override;
	virtual void ProcessInput() override;
	virtual void Update() override;
private:
	void RefreshRoomListUI();
private:
	std::shared_ptr<InputTextBox> m_pRoomNameInputBox = nullptr;
	std::shared_ptr<TextBox>      m_pStatusText = nullptr;
	struct RoomUIItem {
		std::shared_ptr<TextButton> pJoinButton;
		int roomId;
	};
	std::vector<RoomUIItem> m_RoomUIItems;
	float m_fRefreshTimer = 0.f;
	int m_nSelectedRoomId = -1;
	bool m_bProceedToLobby = false;
};
