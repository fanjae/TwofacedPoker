#pragma once
#include "Game_Manager.h"
#include "Client_Handle.h"

Game_Manager::Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager) : roomNumber(roomNumber), roomManager(roomManager), isGamePlaying(false), deck(), dealerchips(0) {}
bool Game_Manager::Handle_Game_Event(const SOCKET socket, const std::string& message)
{
	if (message.substr(0, GAME_START.length()) == GAME_START)
	{
		Handle_Game_Start(socket);
	}
	return true;
}
void Game_Manager::Handle_Game_Start(const SOCKET socket)
{
	std::cout << "[System] : Game_Start Event " << std::endl;
	std::string send_message;
	if (roomManager->All_User_Start_Ready_State())
	{
		send_message = GAME_CLIENT_EVENT + START + DONE;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

		gameInit(socket, InitType::INIT);

		send_message = GAME_CLIENT_EVENT + GAME_INIT;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

		send_message = GAME_CLIENT_EVENT + TURN + MY;
		roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

		send_message = GAME_CLIENT_EVENT + TURN + OTHER;
		roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);

	}
	else
	{
		send_message = GAME_CLIENT_EVENT + START + READY;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);
	}
}

void Game_Manager::gameInit(const SOCKET socket, InitType init_type)
{
	for (auto user : users)
	{
		if (init_type == InitType::INIT)
		{
			user.second->setchips(DEFAULT_CHIPS);
		}
		user.second->setFrontBet(0);
		user.second->setBackBet(0);
		user.second->setFrontCard(0);
		user.second->setBackCard(0);
	}
	setdealerchips(0);
}
bool Game_Manager::getisGamePlaying()
{
	return this->isGamePlaying;
}
void Game_Manager::setisGamePlaying(bool value)
{
	this->isGamePlaying = value;
}
int Game_Manager::getdealerchips()
{
	return this->dealerchips;
}
void Game_Manager::setdealerchips(int value)
{
	this->dealerchips = value;
}