#pragma once
#include <iostream>
#include <string>
#include <winsock2.h>
#include <map>
#include <set>
#include <mutex>

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


