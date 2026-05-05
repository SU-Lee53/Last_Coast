#pragma once

constexpr short PORT = 9000;
//constexpr int WORLD_WIDTH = 8;
//constexpr int WORLD_HEIGHT = 8;
constexpr int MAX_PLAYERS = 3;
constexpr int MAX_ROOMS = 300;
constexpr int MAX_NAME_LEN = 20;
constexpr int BUF_SIZE = 200;

enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

enum PACKET_TYPE { C2S_LOGIN, C2S_MOVE, S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER };
enum DIRECTION {
	UP = 1 << 0,
	DOWN = 1 << 1,
	LEFT = 1 << 2,
	RIGHT = 1 << 3
};

#pragma pack(push, 1) // Ensure no padding between struct members
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
#pragma pack(pop) // Restore default packing
