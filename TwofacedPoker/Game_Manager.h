#pragma once
#include "deck.h"
#include "Room_Manager.h"
#include <string>
#include <map>
#include <winsock2.h>

const std::string GAME_READY = "Game_Ready ";
const std::string GAME_START = "/Game_Start";
const std::string GAME_CLIENT_EVENT = "/Game_Client_Event ";


class Game_Manager
{
private:
	int roomNumber;
	std::shared_ptr<Room_Manager> roomManager;
	std::map<std::string, std::shared_ptr<User>> users;
	Deck deck;

public:
	Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager);
	bool Handle_Game_Event(const SOCKET& socket, const std::string& message);
	void Handle_Game_Ready(const SOCKET& socket, const std::string& message);
};