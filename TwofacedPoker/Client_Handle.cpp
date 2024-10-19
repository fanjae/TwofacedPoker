#include "Client_Handle.h"
#include "Game_Manager.h"
#include "Room_Manager.h"
#include "Packet.h"

static std::map<int, std::shared_ptr<Game_Manager>> gameManagers;
static std::map<int, std::shared_ptr<Room_Manager>> roomManagers;

static int clientNumber = 1;
static int roomcountNumber = 1;

std::mutex chatRoom_mutex;
std::mutex Room_mutex;

ClientEventHandler::ClientEventHandler(SOCKET clientSocket) : socket(clientSocket) {}
bool ClientEventHandler::handleMessage(const std::string& message)
{
	if (message.substr(0, GET_CHATTING_ROOM.length()) == GET_CHATTING_ROOM)
	{
		Handle_Get_Chatting_Room();
	}
	else if (message.substr(0, EXIT_ROOM.length()) == EXIT_ROOM)
	{
		Handle_Exit_Room();
	}
	else if (message.substr(0, LOGIN.length()) == LOGIN)
	{
		Handle_Login();
	}
	else if (message.substr(0, USER_UPDATE.length()) == USER_UPDATE)
	{
		Handle_User_Update();
	}
	else if (message.substr(0, CREATE_CHATTING_ROOM.length()) == CREATE_CHATTING_ROOM)
	{
		Handle_Create_Chatting_Room(message.substr(CREATE_CHATTING_ROOM.length()));
	}
	else if (message.substr(0, JOIN_CHATTING_ROOM.length()) == JOIN_CHATTING_ROOM)
	{
		Handle_Join_Chatting_Room(message.substr(JOIN_CHATTING_ROOM.length()));
	}
	else if (message.substr(0, CLOSE_SOCKET.length()) == CLOSE_SOCKET)
	{
		std::cout << "[System] : Client disconnected : " << message << std::endl;
		return false;
	}
	else if (message.substr(0, GAME_EVENT.length()) == GAME_EVENT)
	{
		const int temproomNumber = this->roomNumber;
		gameManagers[temproomNumber]->Handle_Game_Event(this->socket, message.substr(GAME_EVENT.length()));
	}
	else
	{
		std::string send_message = "[" + this->ID + "]" + message;
		roomManagers[this->roomNumber]->broadcast_Message(send_message, this->socket, TargetType::ALL);
	}
	return true;
}

void ClientEventHandler::Handle_Get_Chatting_Room()
{
	std::string room_List = "";
	if (roomManagers.empty())
	{
		room_List += NO_ROOM;
	}
	else
	{
		for (auto& room : roomManagers)
		{
			if (room.second->getroomCount() < 2)
			{
				room_List += std::to_string(room.first) + " " + room.second->getroomName() + "\n";
			}
		}
	}
	if (room_List == "")
	{
		room_List += NO_ROOM;
	}
	
	SendPacket(socket, room_List);
}


void ClientEventHandler::Handle_Exit_Room()
{
	std::cout << "[System] : " << "Handle_exit_Room" << std::endl;
	std::map<int, std::shared_ptr<Room_Manager>>::iterator it;

	it = roomManagers.find(this->roomNumber);
	if (it != roomManagers.end())
	{
		std::cout << "[System] : Room Find.";
		it->second->removeUser(this->ID, this->socket);
		if (it->second->isroomEmpty() == true)
		{
			roomManagers.erase(this->roomNumber);
			gameManagers.erase(this->roomNumber);
			this->roomNumber = 0;
		}
	}
	else
	{
		std::cout << "[System] : Room Not Found";
		return ;
	}
}

void ClientEventHandler::Handle_Create_Chatting_Room(const std::string& roomName)
{
	const std::lock_guard<std::mutex> lock(chatRoom_mutex);
	std::string send_message = std::to_string(roomcountNumber);

	std::shared_ptr<Room_Manager> roomManager = Room_Manager::createRoom(roomcountNumber, roomName);
	roomManagers[roomcountNumber] = roomManager;

	std::shared_ptr<Game_Manager> gameManager = std::make_shared<Game_Manager>(roomcountNumber, roomManager);
	gameManagers[roomcountNumber] = gameManager;
	roomcountNumber++;

	std::cout << "[System] : Room_Number : " << send_message << "Room_Name : " << roomName << std::endl;
	SendPacket(this->socket, send_message);
}

void ClientEventHandler::Handle_Join_Chatting_Room(const std::string& roomNumber)
{
	this->roomNumber = stoi(roomNumber);
	std::string send_message = "";
	{
		const std::lock_guard<std::mutex> lock(chatRoom_mutex);
		auto it = roomManagers.find(stoi(roomNumber));
		if(it != roomManagers.end())
		{
			send_message = it->second->getroomName();
			SendPacket(this->socket, send_message);

			it->second->addUser(this->ID, this->socket);
			send_message = this->ID + " Joined.";
			it->second->broadcast_Message(send_message, this->socket, TargetType::OTHERS);
		}
		else
		{
			send_message = NOT_EXIST_ROOM;
			SendPacket(socket, send_message);
		}
	}
}

void ClientEventHandler::Handle_User_Update()
{
	std::cout << "[System] : " << "Handle_exit_Room" << std::endl;
	std::map<int, std::shared_ptr<Room_Manager>>::iterator it;

	it = roomManagers.find(this->roomNumber);
	if (it != roomManagers.end())
	{
		if (it->second->getroomCount() > 1)
		{
			it->second->userUpdate(this->socket);
		}
		else
		{
			std::cerr << "[System] : Room Not Found";
			return;
		}
	}
	else
	{
		std::cerr << "[System] : Room Not Found";
		return;
	}
}

void ClientEventHandler::Handle_Login()
{
	this->ID = "Guest" + std::to_string(clientNumber);
	std::string send_message = "ID : " + this->ID;
	clientNumber++;
	SendPacket(socket, send_message);
}

void ConnectClient(SOCKET clientSocket)
{
	ClientEventHandler handler(clientSocket);
	std::string message;

	std::cout << "[System] : Client Connect! " << "clientSocket : " << clientSocket << std::endl;
	while (true)
	{
		message = ReceivePacket(clientSocket);
		if (message == "") break;
		
		handler.handleMessage(message);
	}
	std::cout << "Bye";
	closesocket(clientSocket);
}