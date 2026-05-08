#pragma once

constexpr short PORT = 9000;
//constexpr int WORLD_WIDTH = 8;
//constexpr int WORLD_HEIGHT = 8;
constexpr int MAX_PLAYERS = 3;
constexpr int MAX_ROOMS = 300;
constexpr int MAX_NAME_LEN = 20;
constexpr int BUF_SIZE = 512;

enum PACKET_TYPE {
	C2S_LOGIN, C2S_MOVE,
	C2S_PLAYER_POSITION,                          // 클라이언트 → 서버: 플레이어 위치/방향 (20Hz)
	S2C_LOGIN_RESULT, S2C_AVATAR_INFO,
	S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER,
	S2C_SPAWN_ZOMBIE,                             // 서버 → 클라이언트: 좀비 스폰
	S2C_DESPAWN_ZOMBIE,                           // 서버 → 클라이언트: 좀비 디스폰
	S2C_ZOMBIE_STATE,                             // 서버 → 클라이언트: 매 틱 위치/방향/행동상태
	S2C_ZOMBIE_ATTACK,                            // 서버 → 클라이언트: 좀비 공격 히트 이벤트
};
enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

enum DIRECTION { UP, DOWN, LEFT, RIGHT };

#pragma pack(push, 1) // Ensure no padding between struct members
struct C2S_Login {
	unsigned char size;
	PACKET_TYPE   type;
	char username[MAX_NAME_LEN];
};

struct C2S_Move {
	unsigned char size;
	PACKET_TYPE   type;
	DIRECTION    dir;
};

struct S2C_LoginResult {
	unsigned char size;
	PACKET_TYPE   type;
	bool success;
	char message[50];
};

struct S2C_AvatarInfo {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	short x;
	short y;
};

struct S2C_AddPlayer {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	char username[MAX_NAME_LEN];
	short x;
	short y;
};

struct S2C_RemovePlayer {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
};

struct S2C_MovePlayer {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	short x;
	short y;
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
};

#pragma pack(pop) // Restore default packing
