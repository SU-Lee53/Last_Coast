#pragma once
#include "pch.h"

class Room {

public:
	Room() {
		players.fill(-1);
	}

	bool					add_player(int id);
	void					remove_player(int id);
	bool					is_full();

public:
	std::array<int, 4>		players;
	int						player_count = 0;
	std::mutex				room_lock;

};
