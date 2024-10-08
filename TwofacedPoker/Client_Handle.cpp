#include "Client_Handle.h"
static std::map<std::string, std::set<SOCKET>> chatRooms;
static int clientNumber = 1;

std::mutex chatRoom_mutex;

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
		Handle_Create_Chatting_Room(message.substr(CREATE_CHATTING_ROOM.length(), message.length()));
	}
	else if (message.substr(0, JOIN_CHATTING_ROOM.length()) == JOIN_CHATTING_ROOM)
	{
		Handle_Join_Chatting_Room(message.substr(JOIN_CHATTING_ROOM.length(), message.length()));
	}
	else if (message.substr(0, CLOSE_SOCKET.length()) == CLOSE_SOCKET)
	{
		std::cout << "[Log] : Client disconnected : " << message << std::endl;
		return false;
	}
	else
	{
		std::cout << "[Log] : roomName : " << this->roomName << std::endl;
		for (SOCKET target_socket : chatRooms[this->roomName])
		{
			send(target_socket, message.c_str(), message.length(), 0);
		}
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
			room_List += room.first + "\n";
		}
	}
	send(socket, room_List.c_str(), room_List.length(), 0);
}
void ClientEventHandler::Handle_Exit_Room()
{
	std::cout << "[Log] : " << "Handle_exit_Room" << std::endl;
	chatRooms[roomName].erase(this->socket);
	const std::lock_guard<std::mutex> lock(chatRoom_mutex);
	if (chatRooms[roomName].empty())
	{
		chatRooms.erase(roomName);
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
			send_message = roomName;
		}
	}
	send(socket, send_message.c_str(), send_message.length(), 0);
}
void ClientEventHandler::Handle_Join_Chatting_Room(const std::string& message)
{
	this->roomName = message;
	std::string send_message = "";
	{
		const std::lock_guard<std::mutex> lock(chatRoom_mutex);
		if (chatRooms.find(roomName) != chatRooms.end())
		{
			send_message = roomName;
			chatRooms[message].insert(socket);
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
	std::string send_message = "ID : Guest" + std::to_string(clientNumber);
	clientNumber++;
	send(socket, send_message.c_str(), send_message.length(), 0);
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