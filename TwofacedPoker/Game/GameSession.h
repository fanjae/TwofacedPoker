#pragma once

#include "deck.h"

#include <array>
#include <utility>
#include <WinSock2.h>

class GameSession
{
public:
    enum class State
    {
        WAITING,
        BETTING
    };

    GameSession();

    bool start(SOCKET firstTurnSocket);

    void reset();

    bool isWaiting() const noexcept;
    bool isBetting() const noexcept;

    bool isCurrentTurn(SOCKET socket) const noexcept;

    bool passTurnTo(SOCKET opponentSocket);

    std::array<std::pair<int, int>, 2> dealCards();

private:
    State state;
    SOCKET currentTurnSocket;
    Deck deck;
};