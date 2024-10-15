#pragma once
#include "deck.h"
#include <string>
#include <winsock2.h>

const std::string GAME_READY = "/Game Ready";
const std::string GAME_START = "/Game Start";


class Game_Manager
{
private:
	std::string roomName;
	Deck deck;
public:
	Game_Manager(const std::string& roomName);
	void Handle_Game_Event(SOCKET& socket, const std::string& message);
};