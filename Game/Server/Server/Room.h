#pragma once
#include "pch.h"

// 방 하나의 최대 인원
constexpr int MAX_ROOM_PLAYERS = 4;

class Room {

public:
	Room() {
		reset();
	}

	void                    reset();
	bool					add_player(int id);
	void					remove_player(int id);
	bool					is_full();

public:
	int                     room_id = -1;
	char                    room_name[MAX_NAME_LEN]{};
	bool                    is_active = false;
	bool                    is_in_game = false;
	std::array<int, MAX_ROOM_PLAYERS>	players;
	int						player_count = 0;
	int						host_id = -1;   // 방장 = 제일 처음 들어온 플레이어. 나가면 남은 인원 중 승계
	std::mutex				room_lock;

};

extern std::array<Room, MAX_ROOMS> rooms;

void init_rooms();
Room* find_empty_room();
