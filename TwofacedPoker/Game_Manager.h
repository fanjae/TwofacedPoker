#pragma once
#include "deck.h"
#include "Room_Manager.h"
#include "Constants.h"
#include <string>
#include <map>
#include <winsock2.h>

class Game_Manager
{
private:
	int roomNumber;
	std::shared_ptr<Room_Manager> roomManager;
	bool isGamePlaying;
	int dealerchips;
	Deck deck;

public:
	Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager);
	bool Handle_Game_Event(const SOCKET socket, const std::string& message);
	void Handle_Game_Start(const SOCKET socket);
	void gameInit(const SOCKET socket, InitType init_type);
	void giveBasicBetting(const SOCKET socket);
	void giveCards(const SOCKET socket, GameType game_type);

	
	bool getisGamePlaying();
	void setisGamePlaying(bool value);
	int getdealerchips();
	void setdealerchips(int value);
};