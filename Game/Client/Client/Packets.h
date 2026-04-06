#pragma once

//struct StartPacket {
//	int id;
//	bool startFlag;
//};
//
//enum PACKET_TYPE {
//	PACKET_TYPE_PLAYER_TRANSFORM,
//	PACKET_TYPE_PLAYER_SHOT,
//	/*...*/
//};
//
//struct PlayerInformData {
//	int score;
//	float hp;
//	bool alive;
//	bool bInvincible;
//};
//
//struct PlayerTransformData {
//	XMFLOAT4X4 mtxPlayerTransform;
//};
//
//struct PlayerShotData {
//	XMFLOAT3 v3RayPosition = Vector3(0.f, 0.f, 0.f);
//	XMFLOAT3 v3RayDirection = Vector3(0.f, 0.f, 0.f);
//};
//
//struct RockData {
//	XMFLOAT4X4 mtxRockTransform;
//	BYTE nrockID;
//	bool nIsAlive;
//};
//
//struct ClientToServerPacket {
//	int id = 0;
//	PlayerTransformData transformData;
//	PlayerShotData shotData;
//	PlayerInformData informData;
//};
//
//#define CLIENT_NUM 3
//
//struct CLIENT {
//	int id;
//	PlayerTransformData transformData;
//	PlayerShotData shotData;
//	PlayerInformData informData;
//	bool flag;
//};
//
//struct ServertoClientPlayerPacket {
//	CLIENT client[CLIENT_NUM];
//};
//
//struct ServertoClientRockPacket {
//	std::array<RockData, 50> rockData;
//	int size;
//};

// ─────────────────────────────────────────────────────────────
// Last Coast 패킷
// ─────────────────────────────────────────────────────────────

#define MAX_PLAYERS 3
#define MAX_ZOMBIES 50

// 플레이어 1명의 상태 (서버 ↔ 클라 공용)
struct PlayerState {
	int        nPlayerID   = -1;
	XMFLOAT4X4 mtxTransform = {};   // 위치 + 회전
	float      fHP         = 100.f;
	bool       bAlive      = true;
	bool       bFired      = false;  // 이 프레임에 발사 여부
	XMFLOAT3   v3RayOrigin = {};     // 발사 레이 시작점
	XMFLOAT3   v3RayDir    = {};     // 발사 레이 방향 (정규화)
};

// 좀비 1마리의 상태 (서버 → 클라)
struct ZombieState {
	int      nZombieID     = -1;
	XMFLOAT3 v3Position    = {};
	float    fYaw          = 0.f;   // Y축 회전 (라디안)
	float    fHP           = 100.f;
	int      nBehaviorState = 0;    // AIBehaviorState 정수 값
	bool     bDying        = false;
	bool     bValid        = false; // 이 슬롯이 실제 좀비인지
};

// 클라 → 서버
struct ClientToServerPacket {
	PlayerState playerState;
};

// 서버 → 클라
struct ServerToClientPacket {
	PlayerState players[MAX_PLAYERS];
	ZombieState zombies[MAX_ZOMBIES];
	int           nZombieCount = 0;
};
