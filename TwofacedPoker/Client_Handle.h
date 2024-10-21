#pragma once
#include <iostream>
#include <string>
#include <winsock2.h>
#include <map>
#include <set>
#include <mutex>

const std::string CLOSE_SOCKET = "/Close_Socket";
const std::string EXIST_ROOM = "/Exist_Room";
const std::string NOT_EXIST_ROOM = "/Not_Exist_Room";
const std::string NO_ROOM = "/No_Room";
const std::string EXIT_ROOM = "/Exit_Room";
const std::string GET_CHATTING_ROOM = "/Get_Chatting_Room";
const std::string CREATE_CHATTING_ROOM = "/Create_Chatting_Room ";
const std::string JOIN_CHATTING_ROOM = "/Join_Chatting_Room ";
const std::string LOGIN = "/Login";
const std::string USER_UPDATE = "/User_Update";

const std::string LOAD_PLAYER = "Load_Player ";

const std::string ROOM_EVENT = "/Room_Event ";
const std::string GAME_EVENT = "/Game_Client_Event ";

void ConnectClient(SOCKET clientSocket);
class ClientEventHandler
{
public:
	int getUserNumber() const
	{
		return this->userNumber;
	}
	int getRoomNumber() const
	{
		return this->roomNumber;
	}
private:
	SOCKET socket;
	int userNumber;
	int roomNumber;
	std::string ID;
	
	void Handle_Get_Chatting_Room();
	void Handle_Exit_Room();
	void Handle_Create_Chatting_Room(const std::string& message);
	void Handle_Join_Chatting_Room(const std::string& message);
	void Handle_User_Update();
	void Handle_Login();
public:
	ClientEventHandler(SOCKET clientSocket);
	bool handleMessage(const std::string& message);	
};


