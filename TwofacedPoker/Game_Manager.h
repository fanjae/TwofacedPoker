#pragma once
#include "deck.h"
#include "Room_Manager.h"
#include <string>
#include <map>
#include <winsock2.h>

const int DEFAULT_CHIPS = 30;
const std::string GAME_READY = "Game_Ready ";
const std::string GAME_START = "Game_Start";
const std::string GAME_CLIENT_EVENT = "/Game_Client_Event ";
const std::string START = "Start ";
const std::string GAME_INIT = "Game_Init";
const std::string TURN = "Turn ";
const std::string MY = "My";
const std::string OTHER = "Other";

enum class InitType
{
	INIT,
	PROGRESS
};

class Game_Manager
{
private:
	int roomNumber;
	std::shared_ptr<Room_Manager> roomManager;
	std::map<int, std::shared_ptr<User>> users;
	bool isGamePlaying;
	int dealerchips;
	Deck deck;

public:
	Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager);
	bool Handle_Game_Event(const SOCKET socket, const std::string& message);
	void Handle_Game_Start(const SOCKET socket);
	void gameInit(const SOCKET socket, InitType init_type);
	bool getisGamePlaying();
	void setisGamePlaying(bool value);
	int getdealerchips();
	void setdealerchips(int value);
};