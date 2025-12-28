#pragma once

struct StartPacket {
	int id;
	bool startFlag;
};

enum PACKET_TYPE {
	PACKET_TYPE_PLAYER_TRANSFORM,
	PACKET_TYPE_PLAYER_SHOT,
	/*...*/
};

struct PlayerInformData {
	int score;
	float hp;
	bool alive;
	bool bInvincible;
};

struct PlayerTransformData {
	XMFLOAT4X4 mtxPlayerTransform;
};

struct PlayerShotData {
	XMFLOAT3 v3RayPosition = Vector3(0.f, 0.f, 0.f);
	XMFLOAT3 v3RayDirection = Vector3(0.f, 0.f, 0.f);
};

struct RockData {
	XMFLOAT4X4 mtxRockTransform;
	BYTE nrockID;
	bool nIsAlive;
};

struct ClientToServerPacket {
	int id = 0;
	PlayerTransformData transformData;
	PlayerShotData shotData;
	PlayerInformData informData;
};

#define CLIENT_NUM 3

struct CLIENT {
	int id;
	PlayerTransformData transformData;
	PlayerShotData shotData;
	PlayerInformData informData;
	bool flag;
};

struct ServertoClientPlayerPacket {
	CLIENT client[CLIENT_NUM];
};

struct ServertoClientRockPacket {
	std::array<RockData, 50> rockData;
	int size;
};
