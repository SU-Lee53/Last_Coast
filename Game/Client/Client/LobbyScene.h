#pragma once
#include "Scene.h"

/*
	- 할 일
		- 1. 캐릭터 4명이 들어갈 위치와 모두를 담을 적절한 카메라 위치를 잡음
		- 2. "현재 본인" 의 위치에 캐릭터/무기 선택 UI 를 넣음
		- 3. Ready 버튼을 넣음
		- 4. 다른 플레이어의 Ready 상태도 확인 가능해야 함

*/

interface IThirdPersonPlayer;
class TextButton;
struct PlayerJoinEvent;

class LobbyScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;

private:
	// 프리뷰가 없는 방 멤버를 빈 슬롯에 생성 (이미 있으면 no-op)
	void TryAddPreviewPlayer(const PlayerJoinEvent& ev);

	bool m_bReadyState = false;
	// 게임 종료 후 재진입 시 조인 큐가 비어 있으므로 방 멤버 스냅샷으로 1회 시딩
	bool m_bPreviewsSeeded = false;

	std::shared_ptr<Camera> m_pViewCamera = nullptr;
	std::shared_ptr<Camera> m_pSwappedPlayerCamera = nullptr;

	struct PlayerPreviewTransform {
		Vector3 v3Position;
		Vector3 v3Orientation;
	};

	std::array<PlayerPreviewTransform, 3> m_PreviewTransforms{
		PlayerPreviewTransform{ Vector3{50.f, 90.f, 0.f},	Vector3{0.f, 0.f, 0.f} },
		PlayerPreviewTransform{ Vector3{-100.f, 90.f, 0.f}, Vector3{0.f, 10.f, 0.f} },
		PlayerPreviewTransform{ Vector3{-250.f, 90.f, 0.f}, Vector3{0.f, 30.f, 0.f} }
	};
	struct LobbyPreviewPlayer {
		std::shared_ptr<NetworkRemoteThirdPersonPlayer> pPlayer;
		std::shared_ptr<TextBox> pNameTag;
		int nPlayerId = -1;
		int nSlotIndex = -1;
		std::wstring wstrUsername;   // 닉네임 (레디 표시 갱신용)
		bool bReady = false;         // 레디 상태
	};
	std::vector<LobbyPreviewPlayer> m_ActivePreviews; std::array<std::shared_ptr<NetworkRemoteThirdPersonPlayer>, 3> m_pPreviewPlayers;

	const Vector3 m_v3CurrentPlayerPos{ 120.f, 33.f, 210.f };
	const Vector3 m_v3CurrentPlayerOrientation{0.f, -45.f, 0.f};

	std::shared_ptr<TextBox> m_pModelNameTextbox = nullptr;
	int32 m_nCurModelIndex = 0;

	// 1/2번 슬롯 주무기 선택 (3번 슬롯은 인게임 권총 고정)
	std::shared_ptr<TextBox> m_pWeapon1NameTextbox = nullptr;
	std::shared_ptr<TextBox> m_pWeapon2NameTextbox = nullptr;
	int32 m_nWeapon1Index = 0;
	int32 m_nWeapon2Index = 1;

	// 방장 전용 게임 시작 버튼 (호스트일 때만 표시)
	std::shared_ptr<TextButton> m_pStartButton = nullptr;
	std::shared_ptr<TextBox>    m_pRoomTitleText = nullptr;
	bool m_bAllReady = false;   // 방 인원 전원 레디 여부 (버튼 활성화 룩 + 클릭 가드)


	bool m_bProceed = false;
	bool m_bLeaveRoom = false;
};

