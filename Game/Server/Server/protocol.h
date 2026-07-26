#pragma once

constexpr short PORT = 9000;
//constexpr int WORLD_WIDTH = 8;
//constexpr int WORLD_HEIGHT = 8;
constexpr int MAX_PLAYERS = 1000;
constexpr int MAX_ROOMS = 300;
constexpr int MAX_NAME_LEN = 20;
constexpr int MAX_CHAT_LEN = 200;
constexpr int BUF_SIZE = 512;

enum PACKET_TYPE {
	C2S_LOGIN, C2S_MOVE,
	C2S_TRANSFORM,
	C2S_PLAYER_SHOOT,                             // 클라이언트 → 서버: 사격 요청
	C2S_REGISTER,                                 // 클라이언트 → 서버: 회원가입 요청
	S2C_LOGIN_RESULT, S2C_AVATAR_INFO,
	S2C_ADD_PLAYER, S2C_REMOVE_PLAYER,
	S2C_TRANSFORM,
	S2C_SPAWN_ZOMBIE,                             // 서버 → 클라이언트: 좀비 스폰
	S2C_DESPAWN_ZOMBIE,                           // 서버 → 클라이언트: 좀비 디스폰
	S2C_ZOMBIE_STATE,                             // 서버 → 클라이언트: 매 틱 위치/방향/행동상태
	S2C_ZOMBIE_ATTACK,                            // 서버 → 클라이언트: 좀비 공격 히트 이벤트
	S2C_SHOOT_RESULT,                             // 서버 → 클라이언트: 사격 판정 결과
	C2S_PLAYER_RELOAD,                            // 클라이언트 → 서버: 재장전 요청
	S2C_PLAYER_RELOAD,                            // 서버 → 클라이언트: 재장전 알림
	C2S_PLAYER_MELEE,                             // 클라이언트 → 서버: 근접공격 요청
	S2C_PLAYER_MELEE,                             // 서버 → 클라이언트: 근접공격 모션 (리모트 애니메이션)
	S2C_MELEE_HIT,                                // 서버 → 클라이언트: 근접공격 좀비 히트 (좀비 1마리당 1패킷)
	C2S_CHAT,                                     // 클라이언트 → 서버: 채팅 메시지
	S2C_CHAT,                                     // 서버 → 클라이언트: 채팅 메시지 브로드캐스트
	C2S_PLAYER_WEAPON,                            // 클라이언트 → 서버: 무기 교체 요청
	S2C_PLAYER_WEAPON,                            // 서버 → 클라이언트: 무기 교체 브로드캐스트
	S2C_GAME_EVENT,                               // 서버 → 클라이언트: 스크립트 게임 이벤트 (폭파/포스트FX 등)
	S2C_REGISTER_RESULT,
	C2S_READY,                                    // 클라이언트 → 서버: 레디 상태 변경
	S2C_READY_STATE,                              // 서버 → 클라이언트: 누군가 레디 상태 변경
	S2C_GAME_START,                               // 서버 → 클라이언트: 방장이 시작 요청 + 전원 레디 확인 → 게임 시작
	C2S_PLAYER_CHARACTER,                         // 클라이언트 → 서버: 캐릭터 모델 선택 통지
	S2C_PLAYER_CHARACTER,                         // 서버 → 클라이언트: 캐릭터 모델 브로드캐스트
	S2C_ESCAPE_STATE,                             // 서버 → 클라이언트: 탈출 시퀀스 상태/남은시간 (UI)
	C2S_PLAYER_ESCAPE,                            // 클라이언트 → 서버: 탈출 키 입력 (누구든 1명)
	S2C_GAME_END,                                 // 서버 → 클라이언트: 게임 종료(탈출 성공) — 전원
	S2C_ZOMBIE_STATE_BATCH,                       // 서버 → 클라이언트: 여러 좀비 상태를 한 패킷에 묶어 전송
	C2S_GAME_START,                               // 클라이언트 → 서버: 방장이 게임 시작 요청 (전원 레디 시 서버가 S2C_GAME_START 브로드캐스트)
	S2C_HOST_CHANGE,                              // 서버 → 클라이언트: 방장(호스트) 플레이어 ID 통지 (입장/방장 퇴장 시)
	S2C_PLAYER_DEATH,                             // 서버 → 클라이언트: 플레이어 사망 통지 (부활까지 남은 시간 포함)
	S2C_PLAYER_RESPAWN,                           // 서버 → 클라이언트: 플레이어 부활 통지 (부활 위치 포함)
	C2S_LOAD_COMPLETE,                            // 클라이언트 → 서버: 게임씬 로딩 완료 통지
	S2C_GAME_BEGIN,                               // 서버 → 클라이언트: 전원 로딩 완료 — 게임플레이 동시 시작
	C2S_ROOM_LIST_REQ,                            // 클라이언트 → 서버: 방 목록 요청
	S2C_ROOM_LIST_RES,                            // 서버 → 클라이언트: 방 목록 응답
	C2S_CREATE_ROOM,                              // 클라이언트 → 서버: 방 생성 요청 (방 제목 포함)
	S2C_CREATE_ROOM_RESULT,                       // 서버 → 클라이언트: 방 생성 결과
	C2S_JOIN_ROOM,                                // 클라이언트 → 서버: 방 참여 요청 (방 ID)
	S2C_JOIN_ROOM_RESULT,                         // 서버 → 클라이언트: 방 참여 결과
	C2S_LEAVE_ROOM,                               // 클라이언트 → 서버: 방 퇴장 요청
	S2C_LEAVE_ROOM_RESULT,                         // 서버 → 클라이언트: 방 퇴장 결과
	C2S_PLAYER_BANDAGE,                           // 클라이언트 → 서버: 붕대 상태 (시작/취소/완료)
	S2C_PLAYER_BANDAGE,                           // 서버 → 클라이언트: 붕대 모션 브로드캐스트 (리모트 애니메이션)
	S2C_PLAYER_HEAL,                              // 서버 → 클라이언트: 회복 확정 (서버 권위 HP)
	C2S_PLAYER_GRENADE,                           // 클라이언트 → 서버: 수류탄 상태 (투척/와인드업/들기/내리기)
	S2C_PLAYER_GRENADE,                           // 서버 → 클라이언트: 수류탄 모션/투척 브로드캐스트 (리모트 재현)
	C2S_GRENADE_EXPLODE,                          // 클라이언트 → 서버: 수류탄 폭발 위치 통지 (던진 본인만)
	S2C_GRENADE_HIT                               // 서버 → 클라이언트: 폭발 AoE 좀비 히트 (좀비 1마리당 1패킷)
};

// ── 게임 이벤트 ID (서버 트리거, 클라 효과 카탈로그) ──────────────────────────
enum GameEventId : int {
	GE_EXPLOSION       = 0,   // 위치(x,y,z)에 폭발 연출 (파티클 + 사운드)
	GE_HORROR_LOOK     = 1,   // (구) 호러 룩. 호환용 — 내부적으로 EP_HORROR 프리셋으로 처리됨
	GE_RESTORE_LOOK    = 2,   // (구) 룩 복구. 호환용 — 내부적으로 EP_DEFAULT 프리셋으로 처리됨
	GE_HELICOPTER_CRASH = 3,  // 헬기 추락 컷씬: 시네마틱 카메라가 추락하는 헬기를 추적. fDuration = 연출 길이(0이면 기본값)
	GE_ENVIRONMENT     = 4,   // 환경 프리셋 전환: presetId 프리셋(포스트FX/안개/시간/앰비언트 전체)으로 fDuration 동안 페이드
	GE_HELICOPTER_ARRIVE = 5, // 구조 헬기 강하·착륙 컷씬(폭발 X). 착륙 후 탈출 시퀀스(2분 서바이벌→키 탈출) 시작. fDuration = 연출 길이
};

// ── 환경 프리셋 ID (GE_ENVIRONMENT 전용) ─────────────────────────────────────
// 한 프리셋이 포스트프로세싱 + 안개 + 시간(스카이박스) + 글로벌 앰비언트 값을 통째로 담는다.
// 실제 값은 클라 GameEvent.cpp 의 GetEnvironmentPreset() 테이블에 정의 — 새 룩은 거기 항목만 추가.
enum EnvironmentPresetId : int {
	EP_DEFAULT = 0,   // 기본 룩 (모든 값 디폴트 복구)
	EP_NIGHT   = 1,   // 야간: 시간 밤으로 + 앰비언트 어둡게 + 차가운 안개
	EP_DAWN    = 2,   // 새벽: 시간 새벽으로 + 따뜻한 톤
	EP_SUNSET  = 3,   // 석양: 시간 저녁으로 + 따뜻한 안개 + 시네마틱 카메라가 지는 해를 바라봄(bWatchSun)
};
enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };


enum DIRECTION {
	UP = 1 << 0,
	DOWN = 1 << 1,
	LEFT = 1 << 2,
	RIGHT = 1 << 3
};

#pragma pack(push, 1)

struct TransformData {
	float m[4][4];
};

struct C2S_Login {
	unsigned char size;
	PACKET_TYPE   type;
	char username[MAX_NAME_LEN];
	char password[MAX_NAME_LEN];
};

struct C2S_Register {
	unsigned char size;
	PACKET_TYPE   type;
	char username[MAX_NAME_LEN];
	char password[MAX_NAME_LEN];
};

struct C2S_Move {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char dir;
};

struct C2S_Transform {
	unsigned char size;
	PACKET_TYPE   type;
	TransformData transform;
	bool          bRunning;
	bool          bAiming;
	float         fAimPitch;
};

struct S2C_LoginResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool success;
	char message[50];
};

struct S2C_RegisterResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool success;
	char message[50];
};

struct RoomInfo {
	int  roomId;
	char roomName[MAX_NAME_LEN];
	int  playerCount;
	int  maxPlayers;
	bool bInGame;
};

struct C2S_RoomListReq {
	unsigned char size;
	PACKET_TYPE   type;
};

constexpr int MAX_ROOM_LIST_ITEMS = 20;

struct S2C_RoomListRes {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char roomCount;
	RoomInfo      rooms[MAX_ROOM_LIST_ITEMS];
};

struct C2S_CreateRoom {
	unsigned char size;
	PACKET_TYPE   type;
	char          roomName[MAX_NAME_LEN];
};

struct S2C_CreateRoomResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool          success;
	int           roomId;
	char          roomName[MAX_NAME_LEN];
	char          message[50];
};

struct C2S_JoinRoom {
	unsigned char size;
	PACKET_TYPE   type;
	int           roomId;
};

struct S2C_JoinRoomResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool          success;
	int           roomId;
	char          roomName[MAX_NAME_LEN];
	char          message[50];
};

struct C2S_LeaveRoom {
	unsigned char size;
	PACKET_TYPE   type;
};

struct S2C_LeaveRoomResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool          success;
};

struct S2C_AvatarInfo {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	TransformData transform;
};

struct C2S_Ready {
	unsigned char size;
	PACKET_TYPE   type;
	bool          bReady;
};

struct S2C_ReadyState {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	bool          bReady;
};

struct S2C_GameStart {
	unsigned char size;
	PACKET_TYPE   type;
};

struct C2S_GameStart {
	unsigned char size;
	PACKET_TYPE   type;
};

// 게임씬 로딩 완료 통지 (클라 → 서버). 서버는 방 전원 수신 시 S2C_GAME_BEGIN 브로드캐스트.
struct C2S_LoadComplete {
	unsigned char size;
	PACKET_TYPE   type;
};

// 전원 로딩 완료 — 게임플레이 동시 시작 (서버 → 클라).
// S2C_GAME_START(씬 로딩 개시)와 구분: 클라는 이 패킷을 받아야 정지 해제.
struct S2C_GameBegin {
	unsigned char size;
	PACKET_TYPE   type;
};

struct S2C_HostChange {
	unsigned char size;
	PACKET_TYPE   type;
	int           hostPlayerId;
};

struct S2C_AddPlayer {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	char username[MAX_NAME_LEN];
	TransformData transform;
	bool          bRunning;
	bool          bAiming;
	float         fAimPitch;
	unsigned char weaponType;   // 현재 장착 무기 (late-join 동기화용)
	bool          bReady;       // 현재 레디 상태
	unsigned char characterType; // 현재 선택 캐릭터 모델 인덱스 (late-join 동기화용)
};

struct S2C_RemovePlayer {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
};

struct S2C_Transform {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	TransformData transform;
	bool          bRunning;
	bool          bAiming;
	float         fAimPitch;
};

// ── 좀비 행동 상태 (AI.h AIBehaviorState 와 동일 순서 유지) ──────────────────
enum ZombieBehaviorState : unsigned char {
	ZBS_Idle        = 0,
	ZBS_Wandering   = 1,
	ZBS_Alert       = 2,
	ZBS_Investigating = 3,
	ZBS_Chasing     = 4,
	ZBS_Attacking   = 5,
};

// ── 클라이언트 → 서버 ────────────────────────────────────────────────────────

struct C2S_PlayerPosition {
	unsigned char size;
	PACKET_TYPE   type;
	float         x, y, z; // 월드 좌표 (cm)
	float         yaw;      // 라디안
};

// ── 서버 → 클라이언트 ────────────────────────────────────────────────────────

struct S2C_SpawnZombie {
	unsigned char size;
	PACKET_TYPE   type;
	int           zombieId;
	float         x, y, z; // 스폰 위치 (cm)
	unsigned char zombieType; // 0=일반, 1=빠른 좀비 (서버 스폰 시 확률 롤)
};

struct S2C_DespawnZombie {
	unsigned char size;
	PACKET_TYPE   type;
	int           zombieId;
};

// 매 틱 전송. XZ는 NavMesh 기준 권위적 위치, Y는 클라이언트 물리가 처리.
// waypointX/Z : 클라이언트 path-follow 목표 (다음 waypoint)
struct S2C_ZombieState {
	unsigned char      size;
	PACKET_TYPE        type;
	int                zombieId;
	float              x, z;           // NavMesh XZ 위치 (cm)
	float              yaw;            // 회전 (라디안)
	float              waypointX, waypointZ; // 다음 waypoint XZ (cm)
	ZombieBehaviorState behaviorState;
};

struct S2C_ZombieAttack {
	unsigned char size;
	PACKET_TYPE   type;
	int           zombieId;
	int           targetPlayerId;
	float         damage;
	unsigned char animIndex; // 공격 모션 인덱스 (0="Zombie Attack", 1="Zombie Attack1") — 서버가 롤, 데미지 패킷에선 무의미
};

// ── 좀비 상태 배치 전송 ──────────────────────────────────────────────────────
// 좀비 1마리 상태 (S2C_ZombieState 페이로드와 동일 필드). pack(1) 기준 25바이트.
struct ZombieStateEntry {
	int                 zombieId;
	float               x, z;                  // NavMesh XZ 위치 (cm)
	float               yaw;                   // 회전 (라디안)
	float               waypointX, waypointZ;  // 다음 waypoint XZ (cm)
	ZombieBehaviorState behaviorState;
};
// 한 패킷에 담는 최대 좀비 수. size 가 unsigned char(≤255) 제약: 6(헤더) + 9*25 = 231.
constexpr int MAX_ZOMBIE_BATCH = 9;
// 여러 좀비 상태를 묶어 보내는 배치 패킷 — WSASend/힙할당 횟수를 ~9배 줄인다.
struct S2C_ZombieStateBatch {
	unsigned char    size;       // 실제 바이트 수 = 6 + count*sizeof(ZombieStateEntry)
	PACKET_TYPE      type;
	unsigned char    count;      // 이 패킷의 좀비 수 (≤ MAX_ZOMBIE_BATCH)
	ZombieStateEntry entries[MAX_ZOMBIE_BATCH];
};

// ── 사격 패킷 ───────────────────────────────────────────────────────────────

// 클라이언트 → 서버: 사격 요청 (카메라 위치 + 조준 방향 + 총구 위치)
struct C2S_PlayerShoot {
	unsigned char size;
	PACKET_TYPE   type;
	float         originX, originY, originZ;  // 카메라 위치 (cm)
	float         dirX, dirY, dirZ;           // 조준 방향 (정규화)
	float         muzzleX, muzzleY, muzzleZ;  // 총구 월드 위치 (cm)
	float         damage;                     // 무기/펠릿 데미지 (샷건은 펠릿당 1패킷)
};

// 서버 → 클라이언트: 사격 판정 결과
struct S2C_ShootResult {
	unsigned char size;
	PACKET_TYPE   type;
	int           shooterPlayerId;
	unsigned char bHit;                       // 0=miss, 1=static, 2=zombie
	float         hitX, hitY, hitZ;           // 탄착점 (cm)
	float         hitNormalX, hitNormalY, hitNormalZ;
	int           hitZombieId;                // 좀비 히트 시 ID, 아니면 -1
	float         damage;
	float         muzzleX, muzzleY, muzzleZ;  // 발사자 총구 위치 (cm)
	float         shootDirX, shootDirY, shootDirZ; // 발사 방향
};

struct C2S_PlayerReload {
	unsigned char size;
	PACKET_TYPE   type;
};

struct S2C_PlayerReload {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
};

// ── 근접공격 패킷 ───────────────────────────────────────────────────────────

// 클라이언트 → 서버: 근접공격 요청 (플레이어 위치 + 전방 방향)
struct C2S_PlayerMelee {
	unsigned char size;
	PACKET_TYPE   type;
	float         originX, originY, originZ;  // 플레이어 월드 위치 (cm)
	float         dirX, dirY, dirZ;           // 전방 방향 (정규화)
};

// 서버 → 클라이언트: 근접공격 모션 (리모트 플레이어 애니메이션용)
struct S2C_PlayerMelee {
	unsigned char size;
	PACKET_TYPE   type;
	int           attackerPlayerId;
};

// 서버 → 클라이언트: 근접공격 좀비 히트 (히트한 좀비 1마리당 1패킷)
struct S2C_MeleeHit {
	unsigned char size;
	PACKET_TYPE   type;
	int           attackerPlayerId;
	int           zombieId;
	float         damage;
	float         hitX, hitY, hitZ;           // 피 이펙트 위치 (cm)
};

// ── 무기 교체 패킷 ──────────────────────────────────────────────────────────

// 클라이언트 → 서버: 무기 교체 요청
struct C2S_PlayerWeapon {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char weaponType;   // WEAPON_TYPE (클라 enum, uint8)
};

// 서버 → 클라이언트: 무기 교체 브로드캐스트 (리모트 플레이어 무기 반영)
struct S2C_PlayerWeapon {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	unsigned char weaponType;
};

// ── 캐릭터 모델 선택 패킷 ────────────────────────────────────────────────────

// 클라이언트 → 서버: 캐릭터 모델 선택 통지
struct C2S_PlayerCharacter {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char characterType; // 캐릭터 모델 인덱스 (클라 g_strCharacterNames 인덱스)
};

// 서버 → 클라이언트: 캐릭터 모델 브로드캐스트 (리모트 플레이어 모델 반영)
struct S2C_PlayerCharacter {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	unsigned char characterType;
};

// ── 채팅 패킷 ───────────────────────────────────────────────────────────────

// 클라이언트 → 서버: 채팅 메시지 전송
struct C2S_Chat {
	unsigned char size;
	PACKET_TYPE   type;
	char          message[MAX_CHAT_LEN];
};

// 서버 → 클라이언트: 채팅 메시지 브로드캐스트 (보낸 사람 정보 포함)
struct S2C_Chat {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	char          username[MAX_NAME_LEN];
	char          message[MAX_CHAT_LEN];
};

// ── 게임 이벤트 패킷 ────────────────────────────────────────────────────────

// 서버 → 클라이언트: 스크립트 게임 이벤트 트리거.
// eventId(GameEventId)로 효과 종류 구분, x/y/z는 월드 위치(cm), fTargetValue/fDuration은 이벤트별 인자.
struct S2C_GameEvent {
	unsigned char size;
	PACKET_TYPE   type;
	int           eventId;            // GameEventId
	float         x, y, z;            // 월드 위치 (cm) — 위치 무관 이벤트는 무시
	float         fTargetValue;       // 목표값 (예: outputScale)
	float         fDuration;          // 페이드 지속(초)
	int           presetId;           // EnvironmentPresetId (GE_ENVIRONMENT 전용, 그 외 무시)
};

// ── 플레이어 사망/부활 패킷 (서버 권위: 서버가 HP 차감/사망 판정/부활 타이머 주관) ──

// 서버 → 클라이언트: 플레이어 사망. 본인이면 관전 모드 진입, 리모트면 관전 후보에서 제외.
struct S2C_PlayerDeath {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	float         fRespawnSeconds;  // 부활까지 남은 시간(초) — 클라 카운트다운 UI용
};

// 서버 → 클라이언트: 플레이어 부활. 본인이면 관전 해제 + HP 회복 + 위치 이동.
struct S2C_PlayerRespawn {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;
	float         x, y, z;          // 부활 위치 (cm)
};

// ── 탈출 시퀀스 패킷 (마지막 체크포인트: 구조 헬기 착륙 후 서버가 주관) ───────────
// 서버 → 클라이언트: 탈출 시퀀스 상태. 서버가 시간을 재서 주기적으로 브로드캐스트.
struct S2C_EscapeState {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char phase;          // 0=서바이벌(카운트다운), 1=탈출 가능
	float         fRemainSeconds; // 남은 서바이벌 시간(초). phase=1이면 0
};

// 클라이언트 → 서버: 탈출 키 입력 요청 (탈출 가능 상태에서 누구든 1명 누르면 됨)
struct C2S_PlayerEscape {
	unsigned char size;
	PACKET_TYPE   type;
};

// 서버 → 클라이언트: 게임 종료(탈출 성공). 전원에게 브로드캐스트.
struct S2C_GameEnd {
	unsigned char size;
	PACKET_TYPE   type;
};

// ── 붕대(회복 아이템) 패킷 ───────────────────────────────────────────────────
// state: 0=캐스트 시작, 1=취소, 2=완료(회복 적용), 3=들기(총 내림), 4=내리기(총 복귀).
// 서버는 모든 state를 그대로 브로드캐스트하고, 2(완료)만 회복을 적용·확정한다.
// 캐스트 4초는 클라이언트가 진행. 3/4는 리모트 손 모션 동기화 전용 (targetPlayerId 무시).

// 클라이언트 → 서버: 붕대 상태 통지
struct C2S_PlayerBandage {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char state;
	int           targetPlayerId;   // 회복 대상 (자기 자신 포함)
};

// 서버 → 클라이언트: 붕대 모션 브로드캐스트 (리모트 플레이어 애니메이션용)
struct S2C_PlayerBandage {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;         // 붕대 감는 플레이어
	int           targetPlayerId;
	unsigned char state;
};

// 서버 → 클라이언트: 회복 확정. 대상 클라이언트가 자기 HP를 fNewHP로 갱신.
struct S2C_PlayerHeal {
	unsigned char size;
	PACKET_TYPE   type;
	int           targetPlayerId;
	int           healerPlayerId;
	float         fNewHP;
};

// ── 수류탄(투척 아이템) 패킷 ─────────────────────────────────────────────────
// state: 0=투척(x~vz 유효), 2=와인드업(좌클릭 꾹 — 뒤로 드는 모션), 3=들기(총 내림), 4=내리기(총 복귀).
// 투사체 물리는 각 클라이언트가 동일 초기조건(위치+속도)으로 로컬 시뮬 (시각 전용).
// 폭발 데미지는 던진 클라이언트가 C2S_GRENADE_EXPLODE 로 폭발 위치를 보내면
// 서버가 AoE 판정 후 S2C_GRENADE_HIT 를 좀비 1마리당 1패킷 브로드캐스트한다.

// grenadeType: 0=프래그(폭발 데미지), 1=디코이(반경 내 좀비 어그로 유인 — 데미지 없음)

// 클라이언트 → 서버: 수류탄 상태 통지
struct C2S_PlayerGrenade {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char state;
	unsigned char grenadeType;  // 0=프래그, 1=디코이
	float         x, y, z;      // 투척 시작 위치 (cm, state=0만 유효)
	float         vx, vy, vz;   // 투척 초기 속도 (cm/s, state=0만 유효)
};

// 서버 → 클라이언트: 수류탄 모션/투척 브로드캐스트 (리모트 플레이어 재현용)
struct S2C_PlayerGrenade {
	unsigned char size;
	PACKET_TYPE   type;
	int           playerId;     // 수류탄 사용 플레이어
	unsigned char state;
	unsigned char grenadeType;  // 0=프래그, 1=디코이
	float         x, y, z;
	float         vx, vy, vz;
};

// 클라이언트 → 서버: 수류탄 폭발 위치 (던진 본인 클라이언트만 전송 — 서버가 AoE 판정)
// 디코이(grenadeType=1)면 데미지 대신 반경 내 좀비를 폭발 지점으로 유인한다.
struct C2S_GrenadeExplode {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char grenadeType;  // 0=프래그, 1=디코이
	float         x, y, z;      // 폭발 위치 (cm)
};

// 서버 → 클라이언트: 폭발 AoE 좀비 히트 확정 (클라 좀비 HP/사망 반영)
struct S2C_GrenadeHit {
	unsigned char size;
	PACKET_TYPE   type;
	int           attackerPlayerId;
	int           zombieId;
	float         damage;
};

#pragma pack(pop) // Restore default packing
