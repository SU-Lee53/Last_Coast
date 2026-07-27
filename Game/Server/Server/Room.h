#pragma once
#include "pch.h"

// 방 하나의 최대 인원
constexpr int MAX_ROOM_PLAYERS = 4;

class GameWorld;

class Room {

public:
	Room() {
		reset();
	}

	void                    reset();
	bool					add_player(int id);
	void					remove_player(int id);
	bool					is_full();

	// ── 방별 게임 월드 (좀비/체크포인트/탈출 — 방마다 독립 인스턴스) ─────────
	// 게임 시작 시 생성, 게임 종료/방 소멸 시 nullptr. 포인터 교체는 room_lock으로
	// 보호하고, 사용자는 shared_ptr 복사본을 들고 쓴다 (틱/IOCP 스레드 안전).
	std::shared_ptr<GameWorld> get_world();
	void                       set_world(std::shared_ptr<GameWorld> world);

public:
	int                     room_id = -1;
	char                    room_name[MAX_NAME_LEN]{};
	bool                    is_active = false;
	bool                    is_in_game = false;
	std::array<int, MAX_ROOM_PLAYERS>	players;
	int						player_count = 0;
	int						host_id = -1;   // 방장 = 제일 처음 들어온 플레이어. 나가면 남은 인원 중 승계
	std::mutex				room_lock;

private:
	std::shared_ptr<GameWorld> game_world;  // room_lock으로 포인터 교체 보호
};

extern std::array<Room, MAX_ROOMS> rooms;

void init_rooms();
Room* find_empty_room();
