#pragma once
#include "protocol.h"

class Session;
class Room;

// ─────────────────────────────────────────────────────────────────────────────
// C2S 패킷 핸들러 모음 — Session::process_packet 에서 위임받는다.
// 게임 상태(좀비/시작 플래그/탈출)는 self.m_room->get_world() 로 방별 월드를
// 해석해 접근한다 (멀티룸 — 방마다 독립 GameWorld). 무상태라 전부 static.
// ─────────────────────────────────────────────────────────────────────────────
class PacketHandlers {
public:
	static void Login(Session& self, const C2S_Login& pkt);
	static void Register(Session& self, const C2S_Register& pkt);
	static void Transform(Session& self, const C2S_Transform& pkt);
	static void Reload(Session& self);
	static void Bandage(Session& self, const C2S_PlayerBandage& pkt);
	static void Grenade(Session& self, const C2S_PlayerGrenade& pkt);
	static void GrenadeExplode(Session& self, const C2S_GrenadeExplode& pkt);
	static void Weapon(Session& self, const C2S_PlayerWeapon& pkt);
	static void Character(Session& self, const C2S_PlayerCharacter& pkt);
	static void Chat(Session& self, const C2S_Chat& pkt);
	static void Ready(Session& self, const C2S_Ready& pkt);
	static void GameStart(Session& self);
	static void LoadComplete(Session& self);
	static void Escape(Session& self);
	static void RoomListReq(Session& self);
	static void CreateRoom(Session& self, const C2S_CreateRoom& pkt);
	static void JoinRoom(Session& self, const C2S_JoinRoom& pkt);
	static void LeaveRoom(Session& self);

	// 로딩 대기 중 전원 로딩 완료 여부 판정 — 완료 시 S2C_GAME_BEGIN 브로드캐스트 + 게임 시작.
	// LoadComplete 수신 시와, 로딩 중 플레이어 이탈(Network::Disconnect) 시 호출.
	static void TryBeginGame(Room* room);
};
