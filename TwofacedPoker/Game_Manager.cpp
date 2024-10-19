#pragma once
#include "Game_Manager.h"
#include "Client_Handle.h"

Game_Manager::Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager) : roomNumber(roomNumber), roomManager(roomManager), deck() {}
bool Game_Manager::Handle_Game_Event(const SOCKET& socket, const std::string& message)
{
	std::cout << message << std::endl;
	if (message.substr(0, GAME_READY.length()) == GAME_READY)
	{
		Handle_Game_Ready(socket, message.substr(GAME_READY.length()));
	}
	return true;
}

void Game_Manager::Handle_Game_Ready(const SOCKET& socket, const std::string& message)
{
	std::string send_message = GAME_CLIENT_EVENT + message;
	std::cout << send_message << std::endl;

	roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
}