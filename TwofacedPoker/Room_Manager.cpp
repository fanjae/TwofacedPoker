#include "Room_Manager.h"
#include "Packet.h"
#include "foundation.h"
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
std::string Room_Manager::getUserIDFromSocket(const SOCKET& socket)
{
	int userNumber = socketUserNumber[socket];
	return users[userNumber]->getID();
}
std::pair<std::string, bool> Room_Manager::getThisUserInfo(int userNumber)
{
	return { users[userNumber]->getID(), users[userNumber]->getisReady() };
}
void Room_Manager::addUser(int userNumber, const std::string& userID, SOCKET socket)
{
	users[userNumber] = std::make_shared<User>(userNumber,userID);
	sockets.insert(socket);
	socketUserNumber[socket] = userNumber;
	roomCountSet("PLUS");
}
void Room_Manager::removeUser(int userNumber, const std::string& userID, SOCKET socket)
{
	std::string exit_message = EXIT_ROOM_COMPLETE;
	broadcast_Message(exit_message, socket,TargetType::SELF);
	exit_message = users[userNumber]->getID() + " has exited.";

	users.erase(userNumber);
	sockets.erase(socket);
	socketUserNumber.erase(socket);
	roomCountSet("MINUS");

	std::cout << "System : " << exit_message << std::endl;
	broadcast_Message(exit_message,socket,TargetType::OTHERS);
}
void Room_Manager::userUpdate(SOCKET clientSocket)
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
void Room_Manager::Handle_User_Ready(SOCKET clientSocket, const std::string& message)
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
