#pragma once
#include "Game_Manager.h"
#include "Client_Handle.h"

Game_Manager::Game_Manager(const int roomNumber, std::shared_ptr<Room_Manager> roomManager) : roomNumber(roomNumber), roomManager(roomManager), isGamePlaying(false), deck() {}
bool Game_Manager::Handle_Game_Event(const SOCKET socket, const std::string& message)
{
	if (message.substr(0, GAME_START.length()) == GAME_START)
	{
		Handle_Game_Start(socket);
	}
	if (message.substr(0, BETTING.length()) == BETTING)
	{
		betUser(socket, message.substr(BETTING.length()));
	}
	/*
	if (message.substr(0, BETTING.length()) == SPECIAL_BETTING)
	{
		
	}*/
	
	
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

		giveBasicBetting(socket,GameType::BET);
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

void Game_Manager::giveBasicBetting(const SOCKET socket, GameType game_type)
{
	std::cout << "[System] Basic Betting Started\n";
	std::string send_message;

	if (game_type == GameType::DRAW)
	{
		roomManager->updateChips(socket, GameType::INIT, 1);
		giveCards(socket, GameType::INIT);
	}
	else
	{
		roomManager->setdealerchips(2);

		send_message = GAME_CLIENT_EVENT + BASIC_BETTING;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

		roomManager->updateChips(socket, GameType::INIT, 1);
		giveCards(socket, GameType::INIT);
	}
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
void Game_Manager::betUser(const SOCKET socket, const std::string& message)
{
	std::string type[5] = { FRONT,BOTH,BACK,DIE,SPECIAL};
	int bet_message;
	int bet_count;
	for (int i = 0; i < 5; i++)
	{
		if (type[i] == message.substr(0,type[i].length()))
		{
			bet_message = i;
			if (type[i] != DIE && type[i] != SPECIAL)
			{
				bet_count = stoi(message.substr(type[i].length()));
			}
			else
			{
				bet_count = 0;
			}
		}
	}
	std::cout << "bet_message : " << bet_message << "bet_count : " << bet_count << std::endl;
	betChip(socket, bet_count,static_cast<BetType>(bet_message));
}
void Game_Manager::betChip(const SOCKET socket, int bet_count, BetType bet_type)
{
	std::string send_message;
	GameType game_type = GameType::IMPOSSIBLE;
	GameType result_type = GameType::IMPOSSIBLE;
	GameType check_win = GameType::IMPOSSIBLE;
	if (bet_type != BetType::DIE && bet_type != BetType::SPECIAL)
	{
		game_type = roomManager->betChips(socket, bet_count, bet_type);
	}
	else
	{
		if (bet_type == BetType::SPECIAL)
		{
			roomManager->specialCase(socket);
		}
		game_type = GameType::CALL;
	}
	
	if(game_type == GameType::RAISE)
	{
		send_message = GAME_CLIENT_EVENT + TURN + OTHER;
		roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

		send_message = GAME_CLIENT_EVENT + TURN + MY;
		roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
	}
	else if(game_type == GameType::CALL)
	{
		send_message = GAME_CLIENT_EVENT + BATTLE;
		roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

		roomManager->printCard(socket);

		result_type = battleCard(socket, bet_type);
		roomManager->updateChips(socket, result_type, 0);

		check_win = roomManager->endCheck(socket);
		if (result_type == GameType::DRAW)
		{
			check_win = GameType::PROGRESS;
		}

		if (check_win == GameType::FINALWIN)
		{
			send_message = GAME_CLIENT_EVENT + GAME_RESULT + FINALWIN;
			roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

			send_message = GAME_CLIENT_EVENT + GAME_RESULT + FINALLOSE;
			roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
		}
		else if (check_win == GameType::FINALLOSE)
		{
			send_message = GAME_CLIENT_EVENT + GAME_RESULT + FINALLOSE;
			roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

			send_message = GAME_CLIENT_EVENT + GAME_RESULT + FINALWIN;
			roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
		}
		else if (check_win == GameType::PROGRESS)
		{
			giveBasicBetting(socket, result_type);
			check_win = roomManager->endCheck(socket);

			if (check_win == GameType::FINALWIN || check_win == GameType::FINALLOSE)
			{
				send_message = GAME_CLIENT_EVENT + SPECIAL + MY;
				roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + SPECIAL + OTHER;
				roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
				return;
			}

			send_message = GAME_CLIENT_EVENT + GAME_INIT;
			roomManager->broadcast_Message(send_message, socket, TargetType::ALL);

			send_message = GAME_CLIENT_EVENT + TURN + OTHER;
			roomManager->broadcast_Message(send_message, socket, TargetType::SELF);

			send_message = GAME_CLIENT_EVENT + TURN + MY;
			roomManager->broadcast_Message(send_message, socket, TargetType::OTHERS);
		}
	}
}

GameType Game_Manager::battleCard(const SOCKET socket, BetType bet_type)
{
	std::string self_message;
	std::string other_message;
	GameType game_type = roomManager->compareCard(socket, bet_type);
	if (game_type == GameType::WIN)
	{
		self_message = GAME_CLIENT_EVENT + GAME_RESULT + WIN;
		other_message = GAME_CLIENT_EVENT + GAME_RESULT + LOSE;
	}
	else if (game_type == GameType::DRAW)
	{
		self_message = GAME_CLIENT_EVENT + GAME_RESULT + DRAW;
		roomManager->broadcast_Message(self_message, socket, TargetType::ALL);
		return GameType::DRAW;
	}
	else if (game_type == GameType::LOSE)
	{
		self_message = GAME_CLIENT_EVENT + GAME_RESULT + LOSE;
		if (bet_type == BetType::DIE)
		{
			other_message = GAME_CLIENT_EVENT + GAME_RESULT + DIE;
		}
		else
		{
			other_message = GAME_CLIENT_EVENT + GAME_RESULT + WIN;
		}
	}
	else if (game_type == GameType::BOTHWIN)
	{
		self_message = GAME_CLIENT_EVENT + GAME_RESULT + BOTHWIN;
		other_message = GAME_CLIENT_EVENT + GAME_RESULT + BOTHLOSE;
	}
	else if (game_type == GameType::BOTHLOSE)
	{
		self_message = GAME_CLIENT_EVENT + GAME_RESULT + BOTHLOSE;
		other_message = GAME_CLIENT_EVENT + GAME_RESULT + BOTHWIN;
	}
	roomManager->broadcast_Message(self_message, socket, TargetType::SELF);
	roomManager->broadcast_Message(other_message, socket, TargetType::OTHERS);
	return game_type;
}


bool Game_Manager::getisGamePlaying()
{
	return this->isGamePlaying;
}
void Game_Manager::setisGamePlaying(bool value)
{
	this->isGamePlaying = value;
}