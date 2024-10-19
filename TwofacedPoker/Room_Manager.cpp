#include "Room_Manager.h"
#include "Packet.h"
#include "foundation.h"
#include <iostream>
Room_Manager::Room_Manager(const int roomNumber, const std::string& roomName) : roomNumber(roomNumber), roomName(roomName), roomCount(0) { }
std::shared_ptr<Room_Manager> Room_Manager::createRoom(const int roomNumber, const std::string& roomName)
{
	return std::make_shared<Room_Manager>(roomNumber, roomName);
}
void Room_Manager::broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type)
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
std::string Room_Manager::getUserIDFromSocket(const SOCKET& socket) const
{
	auto it = socketUserID.find(socket);
	if (it != socketUserID.end())
	{
		return it->second;
	}
	return "";
}
std::pair<std::string, bool> Room_Manager::getOtherUserInfo(const std::string& userID)
{
	for (auto& target_user : users)
	{
		std::string target_userID = target_user.first;
		std::shared_ptr<User>& target_user_ptr = target_user.second;

		if (userID != target_userID)
		{
			return { target_user_ptr->getID(), target_user_ptr->getisReady() };
		}
	}
	return { "", false };
}
std::pair<std::string, bool> Room_Manager::getThisUserInfo(const std::string& userID)
{
	return { users[userID]->getID(), users[userID]->getisReady() };
}
void Room_Manager::addUser(const std::string& userID, SOCKET socket)
{
	users[userID] = std::make_shared<User>(userID);
	sockets.insert(socket);
	socketUserID[socket] = userID;
	roomCountSet("PLUS");
}
void Room_Manager::removeUser(const std::string& userID, SOCKET socket)
{
	std::string exit_message = EXIT_ROOM_COMPLETE;
	broadcast_Message(exit_message, socket,TargetType::SELF);

	users.erase(userID);
	sockets.erase(socket);
	socketUserID.erase(socket);
	roomCountSet("MINUS");

	exit_message = userID + " has exited.";
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
				std::string ID = getUserIDFromSocket(clientSocket);
				std::pair<std::string, bool> userData = getThisUserInfo(ID);

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
				std::string ID = getUserIDFromSocket(target_socket);
				std::pair<std::string, bool> userData = getThisUserInfo(ID);

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
