#pragma once
#include "deck.h"
#include <string>
#include <winsock2.h>

const std::string GAME_READY = "Game_Ready ";
const std::string GAME_START = "/Game_Start";
const std::string GAME_CLIENT_EVENT = "/Game_Client_Event ";
const std::string GAME_PRE_CALL = "Game_Pre_Call";
const std::string GAME_PRE_LOAD = "Game_Pre_Load ";
const std::string GAME_PRE_LOAD_PLAYER = "Game_Pre_Load_Player";
const std::string GAME_PRE_LOAD_ID = "Game_Pre_Load ID ";
const std::string GAME_PRE_LOAD_READY = "Game_Pre_Load Ready";
const std::string GAME_PRE_LOAD_DONE = "Game_Pre_Load Done";
const std::string GAME_PRE_LOAD_ID_DONE = "Pre_Load_ID_Done ";
const std::string GAME_PRE_LOAD_READY_DONE = "Pre_Load_Ready_Done";
const std::string GAME_PRE_LOAD_DONE_DONE = "Pre_Load_Done_Done";

class Game_Manager
{
private:
	std::string roomName;
	Deck deck;
public:
	Game_Manager(const std::string& roomName);
	bool Handle_Game_Event(const SOCKET& socket, const std::string& message);
	void Handle_Game_Ready(const SOCKET& socket, const std::string& message);
	void Handle_Game_Pre_Call(const SOCKET& socket, const std::string& message);
	void Handle_Game_Pre_Load(const SOCKET& socket, const std::string& message);
};