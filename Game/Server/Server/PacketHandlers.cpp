#include "pch.h"
#include "PacketHandlers.h"
#include "GameWorld.h"
#include "Session.h"
#include "Room.h"
#include "DBManager.h"

namespace
{
	// 로그인 실패 응답 전송
	void SendLoginFail(Session& self, const std::string& message)
	{
		S2C_LoginResult res;
		res.success = false;
		strncpy_s(res.message, message.c_str(), sizeof(res.message));
		self.send_packet(S2C_LOGIN_RESULT, res);
	}
}

void PacketHandlers::Login(Session& self, const C2S_Login& pkt)
{
	std::string id(pkt.username);
	std::string pw(pkt.password);

	if (!DB->Login(id, pw)) {
		SendLoginFail(self, "Invalid ID or Password");
		return;
	}

	strncpy_s(self.m_username, pkt.username, MAX_NAME_LEN);

	// 방 할당
	Room* room = find_empty_room();
	if (room == nullptr) {
		SendLoginFail(self, "No Room Available");
		return;
	}

	room->add_player(self.m_id);
	self.m_room = room;

	self.send_login_success();
	self.send_avatar_info();

	// 방에 있는 다른 플레이어들에게 접속 알림 + 기존 인원 목록 수신
	for (int other_id : room->players) {
		if (other_id == -1 || other_id == self.m_id) continue;
		clients[other_id].send_add_player(self.m_id);
		self.send_add_player(other_id);
	}

	// 방장(호스트) ID를 방 전체에 통지 (신규 입장자 포함)
	for (int other_id : room->players) {
		if (other_id == -1) continue;
		clients[other_id].send_host_change(room->host_id);
	}

	// 이미 스폰된 좀비 목록을 신규 클라이언트에게 전송
	for (auto& [nZombieId, zombie] : m_World.GetZombies().GetZombies()) {
		if (!zombie.bAlive || !zombie.pAgent) continue;
		self.send_spawn_zombie(nZombieId, zombie.pAgent->GetPosition());
	}
}

void PacketHandlers::Register(Session& self, const C2S_Register& pkt)
{
	std::string id(pkt.username);
	std::string pw(pkt.password);

	S2C_RegisterResult res;
	if (DB->Register(id, pw)) {
		res.success = true;
		strncpy_s(res.message, "Register Successful", sizeof(res.message));
	} else {
		res.success = false;
		strncpy_s(res.message, "Register Failed (ID Exists?)", sizeof(res.message));
	}
	self.send_packet(S2C_REGISTER_RESULT, res);
}

void PacketHandlers::Transform(Session& self, const C2S_Transform& pkt)
{
	{
		// 틱 스레드가 m_transform을 읽으므로(위치 스냅샷) 락으로 tearing 방지
		std::lock_guard<std::mutex> lg(self.m_state_lock);
		self.m_transform = pkt.transform;
		self.m_bRunning = pkt.bRunning;
		self.m_bAiming = pkt.bAiming;
		self.m_fAimPitch = pkt.fAimPitch;
	}

	self.send_transform_packet(self.m_id);
}

void PacketHandlers::Reload(Session& self)
{
	BroadcastAll([&](Session& cl) { cl.send_player_reload(self.m_id); });
}

void PacketHandlers::Weapon(Session& self, const C2S_PlayerWeapon& pkt)
{
	self.m_weaponType = pkt.weaponType;   // late-join 동기화용으로 저장

	// 무기 교체를 전체 클라이언트에 브로드캐스트 (본인 포함; 클라가 본인 필터)
	BroadcastAll([&](Session& cl) { cl.send_player_weapon(self.m_id, self.m_weaponType); });
}

void PacketHandlers::Character(Session& self, const C2S_PlayerCharacter& pkt)
{
	self.m_characterType = pkt.characterType;   // late-join 동기화용으로 저장

	// 캐릭터 모델 선택을 전체 클라이언트에 브로드캐스트 (본인 포함; 클라가 본인 필터)
	BroadcastAll([&](Session& cl) { cl.send_player_character(self.m_id, self.m_characterType); });
}

void PacketHandlers::Chat(Session& self, const C2S_Chat& pkt)
{
	// 메시지 널 종단 보장
	char msg[MAX_CHAT_LEN];
	memcpy_s(msg, sizeof(msg), pkt.message, MAX_CHAT_LEN);
	msg[MAX_CHAT_LEN - 1] = '\0';

	std::cout << "[Chat] Player[" << self.m_id << "] " << self.m_username << ": " << msg << std::endl;

	// 모든 접속 클라이언트(보낸 사람 포함)에게 브로드캐스트
	BroadcastAll([&](Session& cl) { cl.send_chat(self.m_id, self.m_username, msg); });
}

void PacketHandlers::Ready(Session& self, const C2S_Ready& pkt)
{
	self.m_bReady = pkt.bReady;
	std::cout << "[Ready] Player[" << self.m_id << "] is " << (self.m_bReady ? "READY" : "NOT READY") << std::endl;

	// 방 안의 모든 클라이언트에게 레디 상태 브로드캐스트
	// (자동 시작 없음 — 게임 시작은 방장의 C2S_GAME_START 로만)
	if (self.m_room) {
		for (int other_id : self.m_room->players) {
			if (other_id == -1) continue;
			clients[other_id].send_ready_state(self.m_id, self.m_bReady);
		}
	}
}

void PacketHandlers::GameStart(Session& self)
{
	// 방장만 시작 가능 + 전원 레디여야 시작
	if (!self.m_room) return;

	if (self.m_room->host_id != self.m_id) {
		std::cout << "[Room] Player[" << self.m_id << "] tried to start but is not host (host=" << self.m_room->host_id << ")\n";
		return;
	}

	int nReadyCount = 0;
	int nConnectedCount = 0;
	for (int other_id : self.m_room->players) {
		if (other_id == -1) continue;
		++nConnectedCount;
		if (clients[other_id].m_bReady) ++nReadyCount;
	}

	// 최소 인원 제한 없음 — 방에 들어온 전원이 레디면 시작 가능
	if (nConnectedCount >= 1 && nReadyCount == nConnectedCount) {
		std::cout << "[Room] Host[" << self.m_id << "] started game with " << nConnectedCount << " players.\n";
		for (int other_id : self.m_room->players) {
			if (other_id == -1) continue;
			clients[other_id].send_game_start();
		}
		m_World.StartGame(); // 이제부터 틱 스레드가 좀비 스폰/AI/체크포인트를 돌린다
	}
	else {
		std::cout << "[Room] Host[" << self.m_id << "] start rejected: " << nReadyCount << "/" << nConnectedCount << " ready.\n";
	}
}

void PacketHandlers::Escape(Session&)
{
	// 탈출 키 입력 — 플래그만 세움. 실제 종료 판정/브로드캐스트는 틱 스레드가
	// 탈출 가능 상태(Ready)일 때만 처리한다 (스레드 안전 + 단일 권위).
	m_World.GetEscape().Request();
}
