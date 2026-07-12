#pragma once
#include "protocol.h"

class Session;
class GameWorld;

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
	void Weapon(Session& self, const C2S_PlayerWeapon& pkt);
	void Character(Session& self, const C2S_PlayerCharacter& pkt);
	void Chat(Session& self, const C2S_Chat& pkt);
	void Ready(Session& self, const C2S_Ready& pkt);
	void GameStart(Session& self);
	void Escape(Session& self);

private:
	GameWorld& m_World;
};
