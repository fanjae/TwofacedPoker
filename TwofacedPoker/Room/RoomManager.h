#pragma once
#include "../Common/foundation.h"
#include "../Network/ClientConnection.h"

#include <WinSock2.h>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <array>
#include <optional>



enum class TargetType
{
	SELF,        
	OTHERS,      
	ALL          
};

class RoomManager
{
private:
	std::string roomName;
	std::map<SOCKET,std::shared_ptr<ClientConnection>> connections;
	std::map<int, std::shared_ptr<User>> users;
	std::map<SOCKET, int> socketUserNumber;

	mutable std::mutex roomMutex;

	int dealerChips;

	struct PendingSend
	{
		SOCKET socket;
		std::shared_ptr<ClientConnection> connection;
		std::string userID;
		std::string message;
	};

	void collectOpponentBackCardUnlocked(std::vector<PendingSend>& pendingSends,SOCKET receiverSocket,int backCard);
	void collectWaitUnlocked(std::vector<PendingSend>& pendingSends,SOCKET receiverSocket);
	void collectMyCardUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,const std::pair<int, int>& cards);
	void collectVisibleOpponentCardUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,int frontCard);
	void collectBetUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,const std::string& cardSide,int betAmount);
	void collectChipUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,int chips);
	void collectBroadcastUnlocked(std::vector<PendingSend>& pendingSends,const std::string& message,SOCKET senderSocket,TargetType targetType);
	static void sendPending(const std::vector<PendingSend>& pendingSends);

	std::optional<SOCKET> findOpponentSocketUnlocked(SOCKET socket) const;
	std::shared_ptr<User> findUserBySocketUnlocked(SOCKET socket) const;
	std::shared_ptr<User> findOpponentUserUnlocked(SOCKET socket) const;

	bool deductBetUnlocked(SOCKET socket,int amount,std::vector<PendingSend>& pendingSends);

	void updateBetInfoUnlocked(SOCKET socket, int count, BetType betType, std::vector<PendingSend>& pendingSends);


public:
	explicit RoomManager(const std::string& roomName);
	bool Handle_Room_Event(const SOCKET& socket, const std::string& message);
	static std::shared_ptr<RoomManager> createRoom(const std::string& roomName);
	void broadcast_Message(const std::string& message, SOCKET sender_socket, TargetType target_type);
	
	bool addUser(int userNumber,const std::string& userID,std::shared_ptr<ClientConnection> connection);
	bool removeUser(int userNumber,const std::string& userID,SOCKET socket,bool gameWasRunning);
	void userUpdate(const SOCKET ID);
	void Handle_User_Ready(const SOCKET ID, const std::string& message);
	void resetAllUsers(InitType init_type);
	void updateCards(SOCKET socket, const std::array<std::pair<int, int>, 2>& cardData);
	void printCard(const SOCKET socket);
	BetResult betChips(const SOCKET socket, int count, BetType bet_type);
	RoundResult  compareCard(const SOCKET socket, BetType bet_type);
	MatchResult endCheck(const SOCKET socket);
	void specialCase();

	bool All_User_Start_Ready_State();
	bool isroomEmpty() const;
	int getroomCount() const;
	std::string getroomName() const;

	std::optional<SOCKET> getOpponentSocket(SOCKET socket) const;

	void collectBasicBet(SOCKET socket,int amount);
	void settleRound(SOCKET socket, RoundResult roundResult);
};


