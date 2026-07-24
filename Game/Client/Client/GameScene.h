#pragma once
#include "Scene.h"
#include "ZombiePool.h"

class TextBox;
class InputTextBox;
class ImageBox;
class HelicopterCrashEvent;
class HelicopterDepartEvent;

class GameScene : public Scene {
public:
	void BuildObjects() override;
	void FinalizeBuild() override;

	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void PostProcessInput() override;
	void Update() override;
	void SyncSceneWithServer() override;

private:
	void BuildPauseMenuUI();
	void SetPauseMenuOpen(bool bOpen);
	void RefreshPauseVolumeText();

	Vector3 v3TerrainPos;
	Vector3 v3TerrainRotation = Vector3{ 0,0,0 };

	Vector3 v3PlayerPos;

	void ProcessPlayerShoot();
	void RemoveDeadZombies();
	// 풀에서 좀비 하나를 꺼내 랜덤 NavMesh 위치에 스폰. 풀 고갈 시 아무 것도 안 함.
	void SpawnZombie();
	// 오프라인 드립 스포너: 일정 간격으로 랜덤 스폰 포인트에 1마리씩 최대치까지 (기본 Idle).
	void UpdateOfflineSpawner();

	// 서버에서 수신한 좀비 이벤트(스폰/디스폰/상태/공격)를 매 프레임 처리.
	void ProcessNetworkZombies();
	// 서버에서 수신한 사격 결과를 소비하고 이펙트 출력.
	void ProcessShootResults();
	// 로컬 근접공격 입력 → 온라인 송신 / 오프라인 로컬 판정
	void ProcessPlayerMelee();
	// 서버 근접공격 결과 → 좀비 데미지/피 + 리모트 애니메이션
	void ProcessMeleeResults();
	// 로컬 붕대 요청(좌=자기/우=근처 아군) → 대상 해석 + 4초 캐스트 시작
	void ProcessPlayerBandage();
	// 서버 붕대 모션/회복 확정 → 리모트 애니메이션 + 본인 HP 갱신
	void ProcessBandageResults();
	// 로컬 수류탄 릴리즈(몽타주 notify) → 투사체 스폰 + 온라인 투척 전송
	void ProcessPlayerGrenade();
	// 서버 수류탄 모션/투척/히트 → 리모트 애니메이션 + 투사체 시뮬 + 좀비 데미지
	void ProcessGrenadeResults();
	// 폭발한 수류탄 처리 — 연출 + 데미지(던진 쪽만) + 월드 제거
	void UpdateGrenades();
	// 투사체 생성 + 월드/추적 목록 등록 (bLocallyOwned = 폭발 데미지 판정 주체)
	void SpawnGrenade(const Vector3& position, const Vector3& velocity, bool bLocallyOwned);
	// 투척 초기조건 계산 (투척/궤도 예측 공용) — 가슴 높이 + 카메라 시선
	bool ComputeGrenadeThrowParams(Vector3& outOrigin, Vector3& outVelocity) const;
	// 수류탄 모드 + 우클릭 유지 → 예측 궤도 라인 갱신 (GrenadeArcPass)
	void UpdateGrenadeArcPreview();
	// 서버 스크립트 게임 이벤트(폭파/포스트FX) 소비 → EventSequence에 등록
	void ProcessServerGameEvents();
	// 마지막 체크포인트 후: 구조 헬기 착륙 감지 → 2분 서바이벌 → 헬기 반경 내 F키 탈출
	void UpdateEscapeSequence();

	// ── 사망/관전/부활 ──────────────────────────────────────────────────────
	// 사망 감지(온라인=서버 패킷, 오프라인=로컬 HP) → 관전 진입 → 좌클릭 대상 순환 → 부활
	void UpdateDeathAndSpectate();
	void EnterSpectateMode();
	// respawnPos=nullptr 이면 제자리 부활 (오프라인)
	void LeaveSpectateMode(const Vector3* respawnPos);
	// 다음 살아있는 리모트 플레이어로 카메라 이동. 후보 없으면 내 시체 시점 유지 + false
	bool CycleSpectateTarget();

	// 자체 UI 채팅 (기존 ImGui 채팅 창 대체)
	// 히스토리/입력 UI 컴포넌트 생성, Enter 폴링으로 열기/전송/닫기, 히스토리 갱신
	void BuildChatUI();
	void UpdateChat();
	void BeginEndCredits();
	void BuildEndCreditsUI();
	void UpdateEndCredits();
	void ClearEndCreditsUI();

private:
	std::shared_ptr<class WaterGridObject> m_pWater = nullptr;
	Vector3 m_v3WaterPos{};
	ZombiePool m_ZombiePool;

	// 오프라인 드립 스폰 설정
	static constexpr int   OFFLINE_MAX_ZOMBIES   = 100;   // 동시 존재 최대 마리수
	static constexpr float OFFLINE_SPAWN_INTERVAL = 2.0f; // 스폰 간격 (초)
	float m_fOfflineSpawnTimer = 0.f;
	//std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;

	// serverId → 클라이언트 Zombie 인스턴스 (서버 연결 시 사용)
	std::unordered_map<int, std::shared_ptr<Zombie>> m_ServerZombies;

	// ── 수류탄 투사체 ─────────────────────────────────────────────────────────
	static constexpr float GRENADE_THROW_SPEED  = 1700.f; // 투척 속도 (cm/s)
	static constexpr float GRENADE_THROW_UPWARD = 350.f;  // 위쪽 보정 속도 (cm/s)
	std::vector<std::shared_ptr<GrenadeProjectile>> m_ActiveGrenades;

	// 와인드업 중 마지막 궤도 초기조건 — 릴리즈 notify 시점엔 줌 해제로 카메라가
	// 복귀 중이라 다시 계산하면 표시한 궤도와 어긋난다. 저장값을 그대로 투척에 사용.
	Vector3 m_v3AimedThrowOrigin = Vector3::Zero;
	Vector3 m_v3AimedThrowVel    = Vector3::Zero;
	bool    m_bAimedThrowValid   = false;

	// ── 탈출 시퀀스 (서버 권위: 서버가 타이머/상태/종료 주관, 클라는 UI + 키 전송) ──
	std::shared_ptr<HelicopterCrashEvent> m_pArriveEvent;  // 착륙 이벤트 (GetExtractionPos 폴링)
	Vector3 m_v3ExtractionPos{};         // 착륙 헬기 = 탈출 지점 (착륙 시 1회 기록, 반경 판정용)
	bool    m_bExtractionRecorded = false;
	bool    m_bEscapeKeySent = false;    // F 중복 전송 방지
	bool    m_bGameCleared = false;      // 출발 컷씬 종료 후 클리어 표시
	bool    m_bDeparting = false;        // 출발 컷씬 진행 중
	std::shared_ptr<HelicopterDepartEvent> m_pDepartEvent;  // 출발 컷씬 이벤트 (완료 폴링)
	std::shared_ptr<TextBox> m_pEscapeText;  // 탈출 안내 HUD (총알 UI와 동일한 TextBox 방식)
	static constexpr float ESCAPE_RADIUS = 500.f; // 탈출 가능 반경 (헬기 5m 이내)

	// 붕대 타인 회복 — 대상 탐색 반경 / 캐스트 유지 한계 거리 (cm)
	// 유지 한계는 탐색보다 여유를 둬 경계에서 미세 이동으로 바로 끊기지 않게 한다.
	static constexpr float BANDAGE_TARGET_RANGE = 200.f;
	static constexpr float BANDAGE_BREAK_RANGE  = 300.f;
	// 대상 선택 시야각: 카메라 전방 ±60° 이내만 (cos 60° = 0.5)
	static constexpr float BANDAGE_TARGET_FOV_COS = 0.5f;

	// ── 전원 로딩 동기화 ─────────────────────────────────────────────────────
	// 게임씬 진입 시 서버에 C2S_LOAD_COMPLETE 통지 → S2C_GAME_BEGIN(전원 로딩 완료)
	// 수신까지 시네마틱 정지로 게임플레이(입력/플레이어/좀비) 홀드.
	bool m_bWaitingAllLoaded = false;
	std::shared_ptr<TextBox> m_pLoadWaitText;   // 화면 중앙 "다른 플레이어 대기 중" 안내

	// ── 사망/관전/부활 상태 ──────────────────────────────────────────────────
	static constexpr float OFFLINE_RESPAWN_SECONDS = 10.f; // 오프라인 부활 대기 (온라인은 서버가 줌)
	bool  m_bSpectating       = false;
	int   m_nSpectateTargetId = -1;    // 관전 중인 리모트 playerId (-1 = 대상 없음)
	float m_fRespawnRemain    = 0.f;   // 부활까지 남은 시간(초) — HUD 카운트다운
	std::unordered_set<int> m_DeadPlayers;       // 죽어있는 리모트 (관전 후보 제외)
	std::shared_ptr<TextBox> m_pSpectateText;    // 우상단 관전/부활 HUD

	// 리모트 스폰 시각 — 접속 직후 도착하는 초기 무기 장착 브로드캐스트에
	// 드로우 모션이 재생되지 않도록 유예 판정에 사용 (무기 자체는 항상 갱신)
	std::unordered_map<int, float> m_RemoteSpawnTimes;

	// ── 채팅 UI ──────────────────────────────────────────────────────────────
	static constexpr int    CHAT_VISIBLE_LINES = 8;     // 화면에 표시할 히스토리 줄 수
	static constexpr size_t CHAT_MAX_HISTORY   = 100;   // 보관할 누적 라인 수

	std::shared_ptr<InputTextBox>                       m_pChatInput;
	std::array<std::shared_ptr<TextBox>, CHAT_VISIBLE_LINES> m_pChatLines{};
	std::vector<std::wstring>                           m_ChatHistory;

	// ── Pause Menu ───────────────────────────────────────────────────────────
	bool m_bPauseMenuOpen = false;
	bool m_bReturnToMenu = false;
	bool m_bRestoreMouseLook = false;
	std::vector<std::shared_ptr<IUIComponent>> m_pPauseMenuComponents;
	std::shared_ptr<TextBox> m_pPauseVolumeText;

	// End credits
	bool m_bEndCreditsPlaying = false;
	bool m_bEndCreditsFinished = false;
	float m_fEndCreditsElapsed = 0.0f;
	constexpr static float END_CREDITS_DURATION = 10.0f;
	constexpr static float END_CREDITS_FADE_IN_DURATION = 2.5f;
	constexpr static float END_CREDITS_FIRST_LINE_OFFSET_Y = 120.0f;
	constexpr static float END_CREDITS_LINE_HEIGHT = 34.0f;
	std::shared_ptr<ImageBox> m_pEndBackgroundImage;
	std::shared_ptr<TextBox> m_pEndTitleText;
	std::vector<std::shared_ptr<TextBox>> m_pEndCreditTexts;
};
