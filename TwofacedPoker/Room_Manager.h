#pragma once
#include <string>
#include <WinSock2.h>
#include <set>
#include <map>
#include <memory>
#include "foundation.h"

const std::string EXIT_ROOM_COMPLETE = "/Exit_Room_Complete";
const std::string ROOM_CLIENT_EVENT = "/Room_Event ";
const std::string UPDATE_ID = "Update_ID ";
const std::string UPDATE_READY_STATE = "Update_Ready_State ";
const std::string USER_READY_STATE = "User_Ready_State ";
const std::string READY = "READY";
const std::string DONE = "DONE";

enum class TargetType
{
	SELF,        
	OTHERS,      
	ALL          
};

class Room_Manager
{
private:
	std::string roomName;
	std::set<SOCKET> sockets;
	std::map<int, std::shared_ptr<User>> users;
	std::map<SOCKET, int> socketUserNumber;
	int roomNumber;
	int roomCount;
public:
	Room_Manager(const int roomNumber, const std::string& roomName);
	bool Handle_Room_Event(const SOCKET& socket, const std::string& message);
	static std::shared_ptr<Room_Manager> createRoom(const int roomNumber, const std::string& roomName);
	void broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type);
	std::string getUserIDFromSocket(const SOCKET& socket);
	std::pair<std::string, bool> getThisUserInfo(int userNumber);
	

	void addUser(int userNumber, const std::string& userID, SOCKET ID);
	void removeUser(int userNumber, const std::string& userID, SOCKET ID);
	void userUpdate(SOCKET ID);
	void Handle_User_Ready(SOCKET ID, const std::string& message);

	bool All_User_Start_Ready_State();
	bool isroomEmpty() const;
	int getroomCount() const;
	std::string getroomName() const;
	void roomCountSet(const std::string& type);
};


