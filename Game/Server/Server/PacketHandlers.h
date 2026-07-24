#pragma once
#include "protocol.h"

class Session;
class GameWorld;
class Room;

// ─────────────────────────────────────────────────────────────────────────────
// C2S 패킷 핸들러 모음 — Session::process_packet 에서 위임받는다.
// 게임 상태(좀비/시작 플래그/탈출)는 생성자로 주입받은 GameWorld 를 통해서만 접근.
// ─────────────────────────────────────────────────────────────────────────────
class PacketHandlers {
public:
	explicit PacketHandlers(GameWorld& world) : m_World(world) {}

	void Login(Session& self, const C2S_Login& pkt);
	void Register(Session& self, const C2S_Register& pkt);
	void Transform(Session& self, const C2S_Transform& pkt);
	void Reload(Session& self);
	void Bandage(Session& self, const C2S_PlayerBandage& pkt);
	void Grenade(Session& self, const C2S_PlayerGrenade& pkt);
	void GrenadeExplode(Session& self, const C2S_GrenadeExplode& pkt);
	void Weapon(Session& self, const C2S_PlayerWeapon& pkt);
	void Character(Session& self, const C2S_PlayerCharacter& pkt);
	void Chat(Session& self, const C2S_Chat& pkt);
	void Ready(Session& self, const C2S_Ready& pkt);
	void GameStart(Session& self);
	void LoadComplete(Session& self);
	void Escape(Session& self);

	// 로딩 대기 중 전원 로딩 완료 여부 판정 — 완료 시 S2C_GAME_BEGIN 브로드캐스트 + 게임 시작.
	// LoadComplete 수신 시와, 로딩 중 플레이어 이탈(Network::Disconnect) 시 호출.
	void TryBeginGame(Room* room);

private:
	GameWorld& m_World;
};
