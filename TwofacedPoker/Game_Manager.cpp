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

		giveBasicBetting(socket);
	}
	else
	{
		send_message = GAME_CLIENT_EVENT + START + READY;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);
	}
}

void Game_Manager::gameInit(const SOCKET socket, InitType init_type)
{
	roomManager->resetAllUsers(init_type);
}

void Game_Manager::giveBasicBetting(const SOCKET socket)
{
	std::cout << "[System] Basic Betting Started\n";
	std::string send_message;
	setdealerchips(2);

	send_message = GAME_CLIENT_EVENT + BASIC_BETTING;
	roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

	roomManager->updateChips(socket, GameType::INIT, 1);
	giveCards(socket, GameType::INIT);
}
void Game_Manager::giveCards(const SOCKET socket, GameType game_type)
{
	std::string send_message;
	std::pair <int, int> card_data[2];
	if (deck.cardEmpty())
	{
		deck.resupplyCard();
	}
	else
	{
		card_data[0] = deck.DealCard();
		card_data[1] = deck.DealCard();
		roomManager->updateCards(socket, game_type, card_data);
	}
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