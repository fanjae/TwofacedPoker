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
	std::map<std::string, std::shared_ptr<User>> users;
	std::map<SOCKET, std::string> socketUserID;
	int roomNumber;
	int roomCount;
public:
	Room_Manager(const int roomNumber, const std::string& roomName);
	static std::shared_ptr<Room_Manager> createRoom(const int roomNumber, const std::string& roomName);
	void broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type);
	std::string getUserIDFromSocket(const SOCKET& socket) const;
	std::pair<std::string, bool> getOtherUserInfo(const std::string& userID);
	std::pair<std::string, bool> getThisUserInfo(const std::string& userID);
	

	void addUser(const std::string& userID, SOCKET ID);
	void removeUser(const std::string& userID, SOCKET ID);
	void userUpdate(SOCKET ID);
	bool isroomEmpty() const;
	int getroomCount() const;
	std::string getroomName() const;
	void roomCountSet(const std::string& type);
};


