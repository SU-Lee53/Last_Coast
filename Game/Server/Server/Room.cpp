#include "pch.h"
#include "Room.h"

bool Room::add_player(int id)
{
	std::lock_guard<std::mutex> lg(room_lock);

	if (player_count >= 4) return false;

	for (auto& p : players) {
		if (p == -1) {
			p = id;
			++player_count;
			return true;
		}
	}
	return false;
}
void Room::remove_player(int id)
{
	std::lock_guard<std::mutex> lg(room_lock);

	for (auto& p : players) {
		if (p == id) {
			p = -1;
			--player_count;
			return;
		}
	}
}

bool Room::is_full()
{
	std::lock_guard<std::mutex> lg(room_lock);
	return player_count >= 4;
}
