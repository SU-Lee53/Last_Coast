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
	C2S_TRANSFORM,
	C2S_PLAYER_SHOOT,                             // 클라이언트 → 서버: 사격 요청
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
};
enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

// enum PACKET_TYPE { 
// 	C2S_LOGIN, 
// 	C2S_MOVE, 
// 	C2S_TRANSFORM,
// 	S2C_LOGIN_RESULT, 
// 	S2C_AVATAR_INFO, 
// 	S2C_ADD_PLAYER, 
// 	S2C_REMOVE_PLAYER, 
// 	S2C_MOVE_PLAYER,
// 	S2C_TRANSFORM
// };

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

struct S2C_AvatarInfo {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	TransformData transform;
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

// ── 사격 패킷 ───────────────────────────────────────────────────────────────

// 클라이언트 → 서버: 사격 요청 (카메라 위치 + 조준 방향 + 총구 위치)
struct C2S_PlayerShoot {
	unsigned char size;
	PACKET_TYPE   type;
	float         originX, originY, originZ;  // 카메라 위치 (cm)
	float         dirX, dirY, dirZ;           // 조준 방향 (정규화)
	float         muzzleX, muzzleY, muzzleZ;  // 총구 월드 위치 (cm)
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

#pragma pack(pop) // Restore default packing
