#pragma once
#include "Game_Manager.h"

Game_Manager::Game_Manager(const std::string& roomName) : roomName(roomName), deck() {}
void Game_Manager::Handle_Game_Event(SOCKET& socket, const std::string& message)
{

}
