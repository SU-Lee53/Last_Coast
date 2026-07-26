#include "pch.h"
#include "Room.h"

std::array<Room, MAX_ROOMS> rooms;

void init_rooms()
{
	for (int i = 0; i < MAX_ROOMS; ++i) {
		rooms[i].room_id = i;
	}
}

Room* find_empty_room()
{
	for (auto& room : rooms) {
		if (!room.is_active)
			return &room;
	}
	return nullptr;
}

void Room::reset()
{
	players.fill(-1);
	player_count = 0;
	host_id = -1;
	is_active = false;
	is_in_game = false;
	memset(room_name, 0, sizeof(room_name));
}

bool Room::add_player(int id)
{
	std::lock_guard<std::mutex> lg(room_lock);

	if (player_count >= MAX_ROOM_PLAYERS) return false;

	for (auto& p : players) {
		if (p == -1) {
			p = id;
			++player_count;
			if (host_id == -1)
				host_id = id;   // 첫 입장자 = 방장
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

			// 방장이 나가면 남은 인원 중 첫 번째에게 승계 (아무도 없으면 -1)
			if (host_id == id) {
				host_id = -1;
				for (int remain : players) {
					if (remain != -1) { host_id = remain; break; }
				}
			}

			if (player_count <= 0) {
				players.fill(-1);
				player_count = 0;
				host_id = -1;
				is_active = false;
				is_in_game = false;
				memset(room_name, 0, sizeof(room_name));
			}
			return;
		}
	}
}

bool Room::is_full()
{
	std::lock_guard<std::mutex> lg(room_lock);
	return player_count >= MAX_ROOM_PLAYERS;
}
