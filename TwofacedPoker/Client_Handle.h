#pragma once
#include <iostream>
#include <string>
#include <winsock2.h>
#include <map>
#include <set>
#include <mutex>

const std::string CLOSE_SOCKET = "/Close_Socket";
const std::string COMPLETE_CREATE_ROOM = "/Complete_Create_Room";
const std::string EXIST_ROOM = "/Exist_Room";
const std::string NOT_EXIST_ROOM = "/Not_Exist_Room";
const std::string NO_ROOM = "/No_Room";
const std::string EXIT_ROOM = "/Exit_Room";
const std::string GET_CHATTING_ROOM = "/Get_Chatting_Room";
const std::string CREATE_CHATTING_ROOM = "/Create_Chatting_Room ";
const std::string JOIN_CHATTING_ROOM = "/Join_Chatting_Room ";
const std::string LOGIN = "/Login";
const std::string GAME_EVENT = "/Game ";

void ConnectClient(SOCKET clientSocket);

class ClientEventHandler
{
public:
	std::string getRoomName()
	{
		return this->roomName;
	}
private:
	SOCKET socket;
	std::string ID;
	std::string roomName;
	
	void Handle_Get_Chatting_Room();
	void Handle_Exit_Room();
	void Handle_Create_Chatting_Room(const std::string& message);
	void Handle_Join_Chatting_Room(const std::string& message);
	void Handle_Login();
	void Handle_Room_Message(const std::string& message);
public:
	ClientEventHandler(SOCKET clientSocket);
	bool handleMessage(const std::string& message);
	
};


