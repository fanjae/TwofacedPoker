#pragma once

#include "GameSession.h"
#include "../Common/Foundation.h"
#include "../Room/RoomManager.h"

#include <memory>
#include <mutex>
#include <string>
#include <winsock2.h>

class GameManager
{
private:
    std::shared_ptr<RoomManager> roomManager;

    std::mutex gameMutex;
    GameSession gameSession;

    void resetGameUnlocked();

    bool passTurnToOpponent(SOCKET currentSocket);
    bool finishIfNeeded(SOCKET socket, MatchResult matchResult);

    void finishGame(SOCKET socket,MatchResult finalResult);

    void notifyTurnChanged(SOCKET previousTurnSocket);

    void Handle_Game_Start(SOCKET socket);

    void giveBasicBetting(SOCKET socket);

    void giveCards(SOCKET socket);

    void betUser(SOCKET socket,const std::string& message);

    void betChip(SOCKET socket,int betCount,BetType betType);

    BetResult resolveBet(SOCKET socket,int betCount,BetType betType);

    void processRoundEnd(SOCKET socket,BetType actionType);

    RoundResult resolveRound(SOCKET socket,BetType actionType);

    void broadcastRoundResult(SOCKET socket,RoundResult roundResult,BetType actionType);

    void prepareNextRound(SOCKET socket,RoundResult roundResult);

public:
    explicit GameManager(std::shared_ptr<RoomManager> roomManager);

    bool Handle_Game_Event(SOCKET socket,const std::string& message);

    bool handlePlayerExit(int userNumber,const std::string& userID,SOCKET socket);

    void resetGame();
};