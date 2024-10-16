#include "Client_Handle.h"
#include "Game_Manager.h"

static std::map<std::string, std::set<SOCKET>> chatRooms;
static std::map<std::string, int> Room_count;
static std::map<std::string, std::shared_ptr<Game_Manager>> gameManagers;
static int clientNumber = 1;

std::mutex chatRoom_mutex;

const std::map<std::string, std::set<SOCKET>>& GetChatRooms() {
	return chatRooms;
}

const std::map<std::string, int>& GetRoomCounts() {
	return Room_count;
}

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
		std::cout << "[Log] : Client disconnected : " << message << std::endl;
		return false;
	}
	else if (message.substr(0, GAME_EVENT.length()) == GAME_EVENT)
	{
		const std::string tempRoomName = this->roomName;
		gameManagers[tempRoomName]->Handle_Game_Event(this->socket, message.substr(GAME_EVENT.length()));
	}
	else
	{
		Handle_Room_Message(message);
	}
	return true;
}
void ClientEventHandler::Handle_Get_Chatting_Room()
{
	std::string room_List = "";
	std::cout << "[Log] : " << chatRooms.size() << std::endl;
	if (chatRooms.empty())
	{
		room_List += NO_ROOM;
	}
	else
	{
		for (auto& room : chatRooms)
		{
			if (Room_count[room.first] < 2)
			{
				room_List += room.first + "\n";
				std::cout << room.first << std::endl;
			}
		}
		if (room_List == "")
		{
			room_List += NO_ROOM;
		}
	}
	send(socket, room_List.c_str(), room_List.length(), 0);
}
void ClientEventHandler::Handle_Exit_Room()
{
	std::cout << "[Log] : " << "Handle_exit_Room" << std::endl;
	std::string exit_message = "";
	for (SOCKET target_socket : chatRooms[this->roomName])
	{
		if (socket != target_socket)
		{
			exit_message = this->ID + " exited.";
			send(target_socket, exit_message.c_str(), exit_message.length(), 0);
		}
	}

	const std::lock_guard<std::mutex> lock(chatRoom_mutex);
	chatRooms[roomName].erase(this->socket);
	Room_count[roomName]--;
	if (chatRooms[roomName].empty())
	{
		chatRooms.erase(roomName);
	}
	if (Room_count[roomName] == 0)
	{
		gameManagers.erase(roomName);
		Room_count.erase(roomName);
	}

}
void ClientEventHandler::Handle_Create_Chatting_Room(const std::string& message)
{
	this->roomName = message;
	std::string send_message = "";
	{
		const std::lock_guard<std::mutex> lock(chatRoom_mutex);
		if (chatRooms.find(roomName) != chatRooms.end())
		{
			send_message = EXIST_ROOM;
		}
		else
		{
			std::cout << "[Log] : Client Create Room : " << message << std::endl;

			chatRooms[message].insert(socket);
			Room_count[message] = 0;

			std::shared_ptr<Game_Manager> roomGameManager = std::make_shared<Game_Manager>(roomName);
			gameManagers[roomName] = roomGameManager;

			send_message = roomName;

		}
	}
	send(socket, send_message.c_str(), send_message.length(), 0);
}
void ClientEventHandler::Handle_Join_Chatting_Room(const std::string& message)
{
	this->roomName = message;
	std::string send_message = "";
	std::string join_message = "";
	{
		const std::lock_guard<std::mutex> lock(chatRoom_mutex);
		if (chatRooms.find(roomName) != chatRooms.end())
		{
			send_message = roomName;
			chatRooms[message].insert(socket);
			Room_count[message]++;
			for (SOCKET target_socket : chatRooms[this->roomName])
			{
				if (socket != target_socket)
				{
					join_message = this->ID + " Joined.";
					send(target_socket, join_message.c_str(), join_message.length(), 0);
				
					join_message = GAME_CLIENT_EVENT + LOAD_PLAYER + this->ID;
					std::cout << join_message << std::endl;
					send(target_socket, join_message.c_str(), join_message.length(), 0);
				}
			}
		}
		else
		{
			send_message = NOT_EXIST_ROOM;
		}
	}

	send(socket, send_message.c_str(), send_message.length(), 0);
}

void ClientEventHandler::Handle_Login()
{
	this->ID = "Guest" + std::to_string(clientNumber);
	std::string send_message = "ID : " + this->ID;
	clientNumber++;
	send(socket, send_message.c_str(), send_message.length(), 0);
}

void ClientEventHandler:: Handle_Room_Message(const std::string& message)
{
	std::cout << "[Log] : roomName : " << this->roomName << "Message : " << message << std::endl;
	std::string send_message = "[" + this->ID + "]" + message;

	for (SOCKET target_socket : chatRooms[this->roomName])
	{
		send(target_socket, send_message.c_str(), send_message.length(), 0);
	}
}
void ConnectClient(SOCKET clientSocket)
{
	ClientEventHandler handler(clientSocket);
	char buffer[1024];
	std::string str_buffer;

	std::cout << "[Log] Client Connect! " << "clientSocket : " << clientSocket << std::endl;

	while (true)
	{
		bool message_handle = false;
		int recv_length = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (recv_length <= 0)
		{
			break;
		}
		buffer[recv_length] = '\0';
		str_buffer = buffer;

		message_handle = handler.handleMessage(str_buffer);
		if (!message_handle) break;
	}
	closesocket(clientSocket);
}