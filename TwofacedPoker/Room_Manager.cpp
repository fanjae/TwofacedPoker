#include "Room_Manager.h"
#include "Packet.h"
#include "foundation.h"
#include "Constants.h"
#include <iostream>
Room_Manager::Room_Manager(const int roomNumber, const std::string& roomName) : roomNumber(roomNumber), roomName(roomName), roomCount(0), dealerChips(0) { }
bool Room_Manager::Handle_Room_Event(const SOCKET& socket, const std::string& message)
{
	std::cout << message << std::endl;
	if (message.substr(0, USER_READY_STATE.length()) == USER_READY_STATE)
	{
		Handle_User_Ready(socket, message.substr(USER_READY_STATE.length()));
	}
	return true;
}

std::shared_ptr<Room_Manager> Room_Manager::createRoom(const int roomNumber, const std::string& roomName)
{
	return std::make_shared<Room_Manager>(roomNumber, roomName);
}

void Room_Manager::broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type)
{
	try
	{

		for (SOCKET socket : sockets)
		{
			switch (target_type)
			{
			case TargetType::SELF:
				if (socket == sender_socket)
				{
					std::cout << "[ID] : " << getUserIDFromSocket(socket) << "[Socket] : " << socket << "[Message] : " << message << std::endl;
					SendPacket(socket, message);
				}
				break;
			case TargetType::OTHERS:
				if (socket != sender_socket)
				{
					std::cout << "[ID] : " << getUserIDFromSocket(socket) << "[Socket] : " << socket << "[Message] : " << message << std::endl;
					SendPacket(socket, message);
				}
				break;
			case TargetType::ALL:
				std::cout << "[ID] : " << getUserIDFromSocket(socket) << "[Socket] : " << socket << "[Message] : " << message << std::endl;
				SendPacket(socket, message);
				break;
			}
		}
	}
	catch(std::exception e)
	{
		std::cout << e.what() << std::endl;
	}
}
std::string Room_Manager::getUserIDFromSocket(const SOCKET socket)
{
	int userNumber = socketUserNumber[socket];
	return users[userNumber]->getID();
}
std::pair<std::string, bool> Room_Manager::getThisUserInfo(int userNumber)
{
	return { users[userNumber]->getID(), users[userNumber]->getisReady() };
}
int Room_Manager::getUserNumberFromSocket(const SOCKET socket)
{
	return socketUserNumber[socket];
}
void Room_Manager::addUser(int userNumber, const std::string& userID, SOCKET socket)
{
	std::lock_guard<std::mutex> lock(roomMutex);
	users[userNumber] = std::make_shared<User>(userNumber,userID);
	sockets.insert(socket);
	socketUserNumber[socket] = userNumber;
	roomCountSet("PLUS");
}
void Room_Manager::removeUser(int userNumber, const std::string& userID, SOCKET socket)
{
	std::string exit_message = EXIT_ROOM_COMPLETE;

	std::lock_guard<std::mutex> lock(roomMutex);
	broadcast_Message(exit_message, socket,TargetType::SELF);
	exit_message = users[userNumber]->getID() + " has exited.";

	users.erase(userNumber);
	sockets.erase(socket);
	socketUserNumber.erase(socket);
	roomCountSet("MINUS");

	std::cout << "System : " << exit_message << std::endl;
	broadcast_Message(exit_message,socket,TargetType::OTHERS);
}
void Room_Manager::userUpdate(const SOCKET clientSocket)
{
	std::string update_message;
	if (roomCount > 1)
	{
		for (SOCKET target_socket : sockets)
		{
			if (target_socket == clientSocket)
			{
				int userNumber = socketUserNumber[clientSocket];
				std::pair<std::string, bool> userData = getThisUserInfo(userNumber);

				update_message = ROOM_CLIENT_EVENT + UPDATE_ID + userData.first;
				std::cout << "Update_message : " << update_message << std::endl;
				broadcast_Message(update_message, clientSocket, TargetType::OTHERS);

				if (userData.second == false)
				{
					update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + READY;
				}
				else
				{
					update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + DONE;
				}
				std::cout << "Update_message : " << update_message << std::endl;
				broadcast_Message(update_message, clientSocket, TargetType::OTHERS);
			}
			else
			{
				int userNumber = socketUserNumber[target_socket];
				std::pair<std::string, bool> userData = getThisUserInfo(userNumber);

				update_message = ROOM_CLIENT_EVENT +  UPDATE_ID + userData.first;
				std::cout << "Update_message : " << update_message << std::endl;
				broadcast_Message(update_message, clientSocket, TargetType::SELF);

				if (userData.second == false)
				{
					update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + READY;
				}
				else
				{
					update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + DONE;
				}
				std::cout << "Update_message : " << update_message << std::endl;
				broadcast_Message(update_message, clientSocket, TargetType::SELF);
			}
		}
	}
}
void Room_Manager::Handle_User_Ready(const SOCKET clientSocket, const std::string& message)
{
	std::string update_message;
	int userNumber = socketUserNumber[clientSocket];
	if(roomCount > 1)
	{
		if (message == DONE)
		{
			update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + DONE;
			users[userNumber]->setisReady(true);
			std::cout << "Update_message : " << update_message << std::endl;
		}
		else if (message == READY)
		{
			update_message = ROOM_CLIENT_EVENT + UPDATE_READY_STATE + READY;
			users[userNumber]->setisReady(false);
			std::cout << "Update_message : " << update_message << std::endl;
		}
		broadcast_Message(update_message, clientSocket, TargetType::OTHERS);
	}
}

void Room_Manager::resetAllUsers(InitType init_type) 
{
	std::lock_guard<std::mutex> lock(roomMutex); 
	for (auto& pair : users) {
		auto& user = pair.second;
		if (init_type == InitType::INIT) {
			user->setChips(DEFAULT_CHIPS); 
		}
		user->setFrontBet(0);
		user->setBackBet(0);
		user->setFrontCard(0);
		user->setBackCard(0);
	}
}
void Room_Manager::updateChips(const SOCKET socket, GameType game_type, int chipCount)
{
	std::lock_guard<std::mutex> lock(roomMutex);
	std::string send_message;
	std::shared_ptr<User> user_data[2];

	int myChip = 0;
	int otherChip = 0;
	int user_chip_info[2] = { 0 };
	int vs_chip_info[2] = { 0 };
	int dealerChip = this->dealerChips;

	for (auto& pair : users) 
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			user_data[0] = user;
			if (game_type == GameType::INIT)
			{
				myChip = user->getChips();
				myChip--;
				user->setChips(myChip);
				user->setBetType(BetType::NONE);
			}
			if (game_type == GameType::BET)
			{
				myChip = user->getChips();
				myChip -= chipCount;
				user->setChips(myChip);
			}
			if (game_type != GameType::INIT && game_type != GameType::BET)
			{
				myChip = user->getChips();
				user_chip_info[0] = user->getFrontBet();
				user_chip_info[1] = user->getBackBet();
				user->setFrontBet(0);
				user->setBackBet(0);
				continue;
			}

			send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(myChip);
			broadcast_Message(send_message, socket, TargetType::SELF);

			send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(myChip);
			broadcast_Message(send_message, socket, TargetType::OTHERS);
		}
		else
		{
			user_data[1] = user;
			if (game_type == GameType::INIT)
			{
				otherChip = user->getChips();
				otherChip--;
				user->setChips(otherChip); 
				user->setBetType(BetType::NONE);
			}
			if (game_type == GameType::BET)
			{
				continue;
			}
			if (game_type != GameType::INIT && game_type != GameType::BET)
			{
				otherChip = user->getChips();
				vs_chip_info[0] = user->getFrontBet();
				vs_chip_info[1] = user->getBackBet();
				user->setFrontBet(0);
				user->setBackBet(0);
				continue;
			}

			send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(otherChip);
			broadcast_Message(send_message, socket, TargetType::OTHERS);

			send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(otherChip);
			broadcast_Message(send_message, socket, TargetType::SELF);
		}
	}
	
	if (game_type == GameType::WIN)
	{
		myChip += user_chip_info[0] + user_chip_info[1] + vs_chip_info[0] + vs_chip_info[1] + dealerChip;
		user_data[0]->setChips(myChip);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::SELF);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);
	}
	else if (game_type == GameType::LOSE)
	{
		otherChip += user_chip_info[0] + user_chip_info[1] + vs_chip_info[0] + vs_chip_info[1] + dealerChip;
		user_data[1]->setChips(otherChip);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::SELF);
	}
	else if (game_type == GameType::DRAW)
	{
		dealerChip += user_chip_info[0] + user_chip_info[1] + vs_chip_info[0] + vs_chip_info[1];
		setdealerchips(dealerChip);

		send_message = GAME_CLIENT_EVENT + DEALER + CHIP_UPDATE + std::to_string(dealerChip);
		broadcast_Message(send_message, socket, TargetType::ALL);
	}
	else if (game_type == GameType::BOTHWIN)
	{
		myChip += user_chip_info[0] + user_chip_info[1] + vs_chip_info[0] + vs_chip_info[1] + dealerChip + 10;
		user_data[0]->setChips(myChip);

		otherChip -= 10;
		user_data[1]->setChips(otherChip);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::SELF);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::SELF);
	}
	else if (game_type == GameType::BOTHLOSE)
	{
		myChip -= 10;
		user_data[0]->setChips(myChip);

		otherChip += user_chip_info[0] + user_chip_info[1] + vs_chip_info[0] + vs_chip_info[1] + dealerChip + 10;
		user_data[1]->setChips(otherChip);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::SELF);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(myChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);

		send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::OTHERS);

		send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(otherChip);
		broadcast_Message(send_message, socket, TargetType::SELF);
	}
}

void Room_Manager::updateCards(const SOCKET socket, GameType game_type, std::pair<int, int> (&card_data)[2])
{
	std::lock_guard<std::mutex> lock(roomMutex);
	std::string send_message;

	std::array <int, 2> myCard = { card_data[0].first,card_data[0].second };
	std::array <int, 2> otherCard = { card_data[1].first,card_data[1].second };
	std::array <std::string, 2> positions = { FRONT, BACK };

	for (auto& pair : users) 
	{
		auto& user = pair.second;
		
		if (pair.first == getUserNumberFromSocket(socket))
		{
			user->setFrontCard(myCard[0]);
			user->setBackCard(myCard[1]);
			if (game_type == GameType::INIT)
			{
				for (int i = 0; i < 2; i++)
				{
					send_message = GAME_CLIENT_EVENT + MY + CARD_UPDATE + positions[i] + std::to_string(myCard[i]);
					broadcast_Message(send_message, socket, TargetType::SELF);
					if (i == 0)
					{
						send_message = GAME_CLIENT_EVENT + OTHER + CARD_UPDATE + std::to_string(myCard[i]);
						broadcast_Message(send_message, socket, TargetType::OTHERS);
					}
				}
			}
		}
		else
		{
			user->setFrontCard(otherCard[0]);
			user->setBackCard(otherCard[1]);
			if (game_type == GameType::INIT)
			{
				for (int i = 0; i < 2; i++)
				{
					send_message = GAME_CLIENT_EVENT + MY + CARD_UPDATE + positions[i] + std::to_string(otherCard[i]);
					broadcast_Message(send_message, socket, TargetType::OTHERS);
					if (i == 0)
					{
						send_message = GAME_CLIENT_EVENT + OTHER + CARD_UPDATE + std::to_string(otherCard[i]);
						broadcast_Message(send_message, socket, TargetType::SELF);
					}
				}
			}
		}
	}
}

void Room_Manager::printCard(const SOCKET socket)
{
	std::lock_guard<std::mutex> lock(roomMutex);
	std::string self_message;
	std::string other_message;

	std::array <int, 2> myCard = { 0, 0 };
	std::array <int, 2> otherCard = { 0, 0 };
	BetType bet_type[2] = { BetType::NONE, BetType::NONE };

	for (auto& pair : users)
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			myCard[0] = user->getFrontCard();
			myCard[1] = user->getBackCard();
			bet_type[0] = user->getBetType();
		}
		else
		{
			otherCard[0] = user->getFrontCard();
			otherCard[1] = user->getBackCard();
			bet_type[1] = user->getBetType();
		}
	}

	if (bet_type[0] == BetType::BACK || bet_type[0] == BetType::BOTH)
	{
		other_message = GAME_CLIENT_EVENT + OTHER + PRINT + std::to_string(myCard[1]);
		broadcast_Message(other_message, socket, TargetType::OTHERS);

		if (bet_type[1] == BetType::BACK || bet_type[1] == BetType::BOTH)
		{
			self_message = GAME_CLIENT_EVENT + OTHER + PRINT + std::to_string(otherCard[1]);	
		}
		else
		{
			self_message = GAME_CLIENT_EVENT + WAIT;
		}
		broadcast_Message(self_message, socket, TargetType::SELF);
	}
	else if (bet_type[1] == BetType::BACK || bet_type[1] == BetType::BOTH)
	{
		self_message = GAME_CLIENT_EVENT + OTHER + PRINT + std::to_string(otherCard[1]);
		broadcast_Message(self_message, socket, TargetType::SELF);

		if (bet_type[0] == BetType::BACK || bet_type[0] == BetType::BOTH)
		{
			other_message = GAME_CLIENT_EVENT + OTHER + PRINT + std::to_string(myCard[1]);
		}
		else
		{
			other_message = GAME_CLIENT_EVENT + WAIT;
		}
		broadcast_Message(other_message, socket, TargetType::OTHERS);
	}
}
GameType Room_Manager::betChips(const SOCKET socket, int count, BetType bet_type)
{
	std::string send_message = "";
	std::shared_ptr<User> my_data;

	BetType user_bet_type = { BetType::NONE };
	int user_chip_info[2] = { 0 };
	int vs_chip_info[2] = { 0 };

	for (auto& pair : users) 
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			my_data = user;
			user_bet_type = user->getBetType();
			user_chip_info[0] = max(user->getFrontBet(), user->getBackBet());	
			user_chip_info[1] = user->getChips();
		}
		else
		{
			vs_chip_info[0] = max(user->getFrontBet(), user->getBackBet());
			vs_chip_info[1] = user->getChips();
		}
	}

	if ((count > user_chip_info[1]) || (count + user_chip_info[0] > vs_chip_info[1] + vs_chip_info[0]))
	{
		std::cout << "IMPOSSIBLE case1" << std::endl;
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		broadcast_Message(send_message, socket, TargetType::SELF);
		return GameType::IMPOSSIBLE;
	}
	else if (bet_type == BetType::BOTH && ((count * 2 > user_chip_info[1]) || (count * 2 + user_chip_info[0] > vs_chip_info[0] + vs_chip_info[1])))
	{
		std::cout << "IMPOSSIBLE case2" << std::endl;
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		broadcast_Message(send_message, socket, TargetType::SELF);
		return GameType::IMPOSSIBLE;
	}
	else if (count + user_chip_info[0] < vs_chip_info[0])
	{
		std::cout << "IMPOSSIBLE case3" << std::endl;
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		broadcast_Message(send_message, socket, TargetType::SELF);
		return GameType::IMPOSSIBLE;
	}
	else if (user_bet_type != BetType::NONE && user_bet_type != bet_type)
	{
		std::cout << "IMPOSSIBLE case4" << std::endl;
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		broadcast_Message(send_message, socket, TargetType::SELF);
		return GameType::IMPOSSIBLE;
	}

	if (bet_type == BetType::BOTH)
	{
		updateChips(socket, GameType::BET, count * 2);
	}
	else
	{
		updateChips(socket, GameType::BET, count);
	}
	
	
	updateBetInfo(socket, count, bet_type);
	if (count + user_chip_info[0] > vs_chip_info[0])
	{
		return GameType::RAISE;
	}
	else if(count + user_chip_info[0] == vs_chip_info[0])
	{
		return GameType::CALL;
	}
	return GameType::IMPOSSIBLE;
}

void Room_Manager::updateBetInfo(const SOCKET socket, int count, BetType bet_type)
{
	std::string send_message = "";
	for (auto& pair : users)
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			user->setBetType(bet_type);
			if (BetType::FRONT == bet_type || BetType::BOTH == bet_type)
			{
				user->setFrontBet(user->getFrontBet() + count);
				send_message = GAME_CLIENT_EVENT + MY + BET_UPDATE + FRONT + std::to_string(user->getFrontBet());
				broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + OTHER + BET_UPDATE + FRONT + std::to_string(user->getFrontBet());
				broadcast_Message(send_message, socket, TargetType::OTHERS);
			}
			if (BetType::BACK == bet_type || BetType::BOTH == bet_type)
			{
				user->setBackBet(user->getBackBet() + count);
				send_message = GAME_CLIENT_EVENT + MY + BET_UPDATE + BACK + std::to_string(user->getBackBet());
				broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + OTHER + BET_UPDATE + BACK + std::to_string(user->getBackBet());
				broadcast_Message(send_message, socket, TargetType::OTHERS);
			}
			if (BetType::BOTH == bet_type)
			{
				send_message = GAME_CLIENT_EVENT + MY + BET_UPDATE + BOTH;
				broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + OTHER + BET_UPDATE + BOTH;
				broadcast_Message(send_message, socket, TargetType::OTHERS);
			}
		}
	}
}

GameType Room_Manager::compareCard(const SOCKET socket, BetType bet_type)
{
	BetType bet_type_info[2] = { BetType::NONE, BetType::NONE };
	int user_card_info[2] = { 0 };
	int vs_card_info[2] = { 0 };

	
	for (auto& pair : users)
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			bet_type_info[0] = user->getBetType();
			user_card_info[0] = user->getFrontCard();
			user_card_info[1] = user->getBackCard();
		}
		else
		{
			bet_type_info[1] = user->getBetType();
			vs_card_info[0] = user->getFrontCard();
			vs_card_info[1] = user->getBackCard();
		}
	}

	if (bet_type == BetType::DIE)
	{
		if (bet_type_info[1] == BetType::BOTH)
		{
			return GameType::BOTHLOSE;
		}
		else
		{
			return GameType::LOSE;
		}
	}

	int user_index = (bet_type_info[0] == BetType::FRONT) ? 0 : 1;
	int vs_index = (bet_type_info[1] == BetType::FRONT) ? 0 : 1;
	if (bet_type_info[0] != BetType::BOTH)
	{
		if (bet_type_info[1] == BetType::BOTH)
		{
			if (user_card_info[user_index] < vs_card_info[0] && user_card_info[user_index] < vs_card_info[1])
			{
				return GameType::BOTHLOSE;
			}
			else
			{
				return GameType::WIN;
			}
		}
		else
		{
			if (user_card_info[user_index] > vs_card_info[vs_index])
			{
				return GameType::WIN;
			}
			else if (user_card_info[user_index] < vs_card_info[vs_index])
			{
				return GameType::LOSE;
			}
			else
			{
				return GameType::DRAW;
			}
		}
	}
	else
	{
		if (bet_type_info[1] == BetType::BOTH)
		{
			return GameType::IMPOSSIBLE;
		}
		else
		{
			if (user_card_info[0] > vs_card_info[vs_index] && user_card_info[1] > vs_card_info[vs_index])
			{
				return GameType::BOTHWIN;
			}
			else
			{
				return GameType::WIN;
			}
		}
	}
	return GameType::IMPOSSIBLE;
}

GameType Room_Manager::endCheck(const SOCKET socket)
{
	int user_chip_info[2] = { 0 };
	std::shared_ptr<User> user_data[2];
	for (auto& pair : users)
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			user_data[0] = user;
			user_chip_info[0] = user->getChips();
		}
		else
		{
			user_data[1] = user;
			user_chip_info[1] = user->getChips();
		}
	}

	if (user_chip_info[0] == 0)
	{
		user_data[0]->setisReady(false);
		user_data[1]->setisReady(false);
		return GameType::FINALLOSE;
	}
	if (user_chip_info[1] == 0)
	{
		user_data[0]->setisReady(false);
		user_data[1]->setisReady(false);
		return GameType::FINALWIN;
	}
	else
	{
		return GameType::PROGRESS;
	}
}

void Room_Manager::specialCase(const SOCKET socket)
{
	for (auto& pair : users)
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			if (user->getFrontCard() < user->getBackCard())
			{
				user->setBetType(BetType::BACK);
			}
			else
			{
				user->setBetType(BetType::FRONT);
			}
		}
		else
		{
			if (user->getFrontCard() < user->getBackCard())
			{
				user->setBetType(BetType::BACK);
			}
			else
			{
				user->setBetType(BetType::FRONT);
			}
		}
	}
}




bool Room_Manager::All_User_Start_Ready_State()
{
	bool start = true;
	for (auto user : users)
	{
		if (user.second->getisReady() == false)
		{
			start = false;
		}
	}
	return start;
}
bool Room_Manager::isroomEmpty() const
{
	return sockets.empty();
}
std::string Room_Manager::getroomName() const
{
	return roomName;
}
int Room_Manager::getroomCount() const
{
	return roomCount;
}

void Room_Manager::roomCountSet(const std::string& type)
{
	if (type == "PLUS")
	{
		roomCount++;
	}
	else if (type == "MINUS")
	{
		roomCount--;
	}
}

int Room_Manager::getdealerchips()
{
	return this->dealerChips;
}
void Room_Manager::setdealerchips(int value)
{
	this->dealerChips = value;
}