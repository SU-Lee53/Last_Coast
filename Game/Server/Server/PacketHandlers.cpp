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

		// 첫 위치 보고 = 게임씬 시작 스폰 위치 (체크포인트 이전 부활 지점)
		if (!self.m_bSpawnPosSet) {
			self.m_v3SpawnPos = Vector3{ pkt.transform.m[3][0], pkt.transform.m[3][1], pkt.transform.m[3][2] };
			self.m_bSpawnPosSet = true;
		}
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

void PacketHandlers::Bandage(Session& self, const C2S_PlayerBandage& pkt)
{
	if (self.m_bDead) return;

	// 붕대 모션 상태를 전체 브로드캐스트 (본인 포함; 클라가 본인 필터)
	BroadcastAll([&](Session& cl) { cl.send_player_bandage(self.m_id, pkt.targetPlayerId, pkt.state); });

	if (pkt.state != 2) return;   // 완료(2)만 회복 적용

	if (pkt.targetPlayerId < 0 || pkt.targetPlayerId >= MAX_PLAYERS) return;
	Session& target = clients[pkt.targetPlayerId];
	if (!target.m_is_connected || target.m_bDead) return;

	// m_fHP는 게임 틱 스레드(좀비 공격)와 IOCP 스레드(여기)가 함께 쓴다 —
	// 정렬된 float 단일 쓰기라 tearing 없음, 드문 동시 갱신 손실은 허용.
	constexpr float BANDAGE_HEAL_AMOUNT = 50.f;
	constexpr float BANDAGE_MAX_HP      = 100.f;
	target.m_fHP = std::min(BANDAGE_MAX_HP, target.m_fHP + BANDAGE_HEAL_AMOUNT);

	BroadcastAll([&](Session& cl) { cl.send_player_heal(pkt.targetPlayerId, self.m_id, target.m_fHP); });
}

void PacketHandlers::Grenade(Session& self, const C2S_PlayerGrenade& pkt)
{
	if (self.m_bDead) return;

	// 수류탄 모션/투척 상태를 전체 브로드캐스트 (본인 포함; 클라가 본인 필터)
	// state 0(투척)은 위치+속도가 담겨 있어 리모트 클라가 동일 초기조건으로 투사체를 시뮬한다.
	BroadcastAll([&](Session& cl) { cl.send_player_grenade(self.m_id, pkt); });
}

void PacketHandlers::GrenadeExplode(Session& self, const C2S_GrenadeExplode& pkt)
{
	// 폭발 연출은 각 클라이언트가 로컬 시뮬 퓨즈로 재생 — 서버는 데미지 판정만.
	// 반경 내 좀비에게 거리 선형 감쇠 데미지, 좀비 1마리당 S2C_GRENADE_HIT 1패킷 (밀리 히트와 동일 방식).
	constexpr float GRENADE_RADIUS     = 700.f;   // AoE 반경 (cm) — 클라 EXPLODE_RADIUS와 일치
	constexpr float GRENADE_DAMAGE_MAX = 150.f;   // 중심 데미지
	constexpr float GRENADE_DAMAGE_MIN = 30.f;    // 반경 끝 데미지

	const Vector3 v3Center{ pkt.x, pkt.y, pkt.z };
	auto& zombies = m_World.GetZombies();
	for (auto& [nZombieId, zombie] : zombies.GetZombies()) {
		if (!zombie.bAlive || !zombie.pAgent) continue;

		const float fDist = Vector3::Distance(zombie.pAgent->GetPosition(), v3Center);
		if (fDist > GRENADE_RADIUS) continue;

		const float fDamage = GRENADE_DAMAGE_MAX
			+ (GRENADE_DAMAGE_MIN - GRENADE_DAMAGE_MAX) * (fDist / GRENADE_RADIUS);
		zombies.ApplyDamageToZombie(nZombieId, fDamage);
		BroadcastAll([&](Session& cl) { cl.send_grenade_hit(self.m_id, nZombieId, fDamage); });
	}
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
		std::cout << "[Room] Host[" << self.m_id << "] started loading with " << nConnectedCount << " players.\n";

		// S2C_GAME_START = 씬 로딩 개시 신호일 뿐, 게임은 아직 시작하지 않는다.
		// 전원이 C2S_LOAD_COMPLETE 를 보내면 TryBeginGame 이 S2C_GAME_BEGIN + StartGame.
		for (int other_id : self.m_room->players) {
			if (other_id == -1) continue;
			clients[other_id].m_bLoadComplete = false;
		}
		m_World.SetAwaitingLoads(true);

		for (int other_id : self.m_room->players) {
			if (other_id == -1) continue;
			clients[other_id].send_game_start();
		}
	}
	else {
		std::cout << "[Room] Host[" << self.m_id << "] start rejected: " << nReadyCount << "/" << nConnectedCount << " ready.\n";
	}
}

void PacketHandlers::LoadComplete(Session& self)
{
	if (!self.m_room) return;

	// 게임이 이미 시작된 뒤 도착한 로딩 완료(지각 진입) — 본인만 즉시 시작시킨다
	if (m_World.IsGameStarted()) {
		self.send_game_begin();
		return;
	}

	self.m_bLoadComplete = true;
	std::cout << "[Room] Player[" << self.m_id << "] load complete.\n";
	TryBeginGame(self.m_room);
}

void PacketHandlers::TryBeginGame(Room* room)
{
	// 시작 요청으로 로딩 대기 중일 때만 판정 (로비 이탈 등 무관한 호출은 무시)
	if (!room || !m_World.IsAwaitingLoads() || m_World.IsGameStarted()) return;

	int nLoadedCount = 0;
	for (int other_id : room->players) {
		if (other_id == -1) continue;
		if (!clients[other_id].m_bLoadComplete) return; // 아직 로딩 중인 플레이어 존재
		++nLoadedCount;
	}

	m_World.SetAwaitingLoads(false);

	// 로딩 중 전원 이탈 — 시작 취소
	if (nLoadedCount == 0) {
		std::cout << "[Room] All players left during loading. Game start canceled.\n";
		return;
	}

	std::cout << "[Room] All " << nLoadedCount << " players loaded. Game begin!\n";
	for (int other_id : room->players) {
		if (other_id == -1) continue;
		clients[other_id].send_game_begin();
	}
	m_World.StartGame(); // 이제부터 틱 스레드가 좀비 스폰/AI/체크포인트를 돌린다
}

void PacketHandlers::Escape(Session& self)
{
	// 죽은(관전 중) 플레이어의 탈출 요청은 무시 — 클라도 막지만 서버가 최종 권위.
	if (self.m_bDead) return;

	// 탈출 키 입력 — 플래그만 세움. 실제 종료 판정/브로드캐스트는 틱 스레드가
	// 탈출 가능 상태(Ready)일 때만 처리한다 (스레드 안전 + 단일 권위).
	m_World.GetEscape().Request();
}
