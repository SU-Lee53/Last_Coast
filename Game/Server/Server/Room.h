#pragma once
#include "pch.h"

// 방 하나의 최대 인원
constexpr int MAX_ROOM_PLAYERS = 4;

class Room {

public:
	Room() {
		players.fill(-1);
	}

	bool					add_player(int id);
	void					remove_player(int id);
	bool					is_full();

public:
	std::array<int, MAX_ROOM_PLAYERS>	players;
	int						player_count = 0;
	int						host_id = -1;   // 방장 = 제일 처음 들어온 플레이어. 나가면 남은 인원 중 승계
	std::mutex				room_lock;

};

extern std::array<Room, MAX_ROOMS> rooms;

// 자리가 남은 첫 번째 방 반환 (없으면 nullptr)
Room* find_empty_room();
