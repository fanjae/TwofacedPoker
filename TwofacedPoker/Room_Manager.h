#pragma once
#include <string>
#include <WinSock2.h>
#include <set>
#include <map>
#include <memory>
#include <mutex>
#include <array>
#include "foundation.h"


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
	std::mutex roomMutex;

	int roomNumber;
	int roomCount;
public:
	Room_Manager(const int roomNumber, const std::string& roomName);
	bool Handle_Room_Event(const SOCKET& socket, const std::string& message);
	static std::shared_ptr<Room_Manager> createRoom(const int roomNumber, const std::string& roomName);
	void broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type);
	std::string getUserIDFromSocket(const SOCKET socket);
	std::pair<std::string, bool> getThisUserInfo(int userNumber);
	int getUserNumberFromSocket(const SOCKET socket);
	

	void addUser(int userNumber, const std::string& userID, SOCKET ID);
	void removeUser(int userNumber, const std::string& userID, SOCKET ID);
	void userUpdate(const SOCKET ID);
	void Handle_User_Ready(const SOCKET ID, const std::string& message);
	void resetAllUsers(InitType init_type);
	void updateChips(const SOCKET ID, GameType game_type, int chipCount);
	void updateCards(const SOCKET ID, GameType game_type, std::pair<int, int>(&card_data)[2]);
	void updateBetInfo(const SOCKET ID, int count, BetType bet_type);
	bool betChips(const SOCKET socket, int count, BetType bet_type);
	

	bool All_User_Start_Ready_State();
	bool isroomEmpty() const;
	int getroomCount() const;
	std::string getroomName() const;
	void roomCountSet(const std::string& type);


};


