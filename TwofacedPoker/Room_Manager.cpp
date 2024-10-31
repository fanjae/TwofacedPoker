#include "Room_Manager.h"
#include "Packet.h"
#include "foundation.h"
#include "Constants.h"
#include <iostream>
Room_Manager::Room_Manager(const int roomNumber, const std::string& roomName) : roomNumber(roomNumber), roomName(roomName), roomCount(0) { }
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

void Room_Manager::resetAllUsers(InitType init_type) {
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
	int myChip = 0;
	int otherChip = 0;
	for (auto& pair : users) 
	{
		auto& user = pair.second;
		if (pair.first == getUserNumberFromSocket(socket))
		{
			if (game_type == GameType::INIT)
			{
				myChip = user->getChips();
				myChip--;
				user->setChips(myChip - 1);
			}
			if (game_type == GameType::BET)
			{
				myChip = user->getChips();
				myChip -= chipCount;
				user->setChips(myChip - chipCount);
			}
			send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(myChip);
			broadcast_Message(send_message, socket, TargetType::SELF);

			send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(myChip);
			broadcast_Message(send_message, socket, TargetType::OTHERS);
		}
		else
		{
			if (game_type == GameType::INIT)
			{
				otherChip = user->getChips();
				otherChip--;
				user->setChips(otherChip - 1); 
			}
			send_message = GAME_CLIENT_EVENT + MY + CHIP_UPDATE + std::to_string(otherChip);
			broadcast_Message(send_message, socket, TargetType::OTHERS);

			send_message = GAME_CLIENT_EVENT + OTHER + CHIP_UPDATE + std::to_string(otherChip);
			broadcast_Message(send_message, socket, TargetType::SELF);
		}
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

bool Room_Manager::betChips(const SOCKET socket, int count, BetType bet_type)
{
	std::string send_message = "";
	std::shared_ptr<User> my_data;

	BetType user_bet_type;
	int user_chip_info[2];
	int vs_chip_info[2];

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

	if (count > user_chip_info[1] || count > vs_chip_info[1])
	{
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		return false;
	}
	else if (bet_type == BetType::BOTH && (count * 2 > user_chip_info[1] || count * 2 > vs_chip_info[1]))
	{
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		return false;
	}
	else if (count + user_chip_info[0] < vs_chip_info[0])
	{
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		return false;
	}
	else if (user_bet_type == BetType::NONE || user_bet_type != bet_type)
	{
		send_message = GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE;
		return false;
	}
	updateChips(socket, GameType::BET, count);
	updateBetInfo(socket, count, bet_type);
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
				send_message = GAME_CLIENT_EVENT + MY + BET_UPDATE + FRONT +  std::to_string(count);
				broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + OTHER + BET_UPDATE + FRONT + std::to_string(count);
				broadcast_Message(send_message, socket, TargetType::OTHERS);
			}
			if (BetType::BACK == bet_type || BetType::BOTH == bet_type)
			{
				user->setBackBet(user->getBackBet() + count);
				send_message = GAME_CLIENT_EVENT + MY + BET_UPDATE + BACK + std::to_string(count);
				broadcast_Message(send_message, socket, TargetType::SELF);

				send_message = GAME_CLIENT_EVENT + OTHER + BET_UPDATE + BACK + std::to_string(count);
				broadcast_Message(send_message, socket, TargetType::OTHERS);
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