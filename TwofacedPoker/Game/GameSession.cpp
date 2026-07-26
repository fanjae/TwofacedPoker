#include "GameSession.h"

// 초기 상태는 WAITING, 턴을 가지는 소켓은 유효하지 않음(INVALID)
GameSession::GameSession() : state(State::WAITING),currentTurnSocket(INVALID_SOCKET), deck()
{
}

// 게임 시작 : 첫 번째 턴을 지정하고, 상태를 BETTING으로 변경
bool GameSession::start(SOCKET firstTurnSocket)
{
    if (state != State::WAITING || firstTurnSocket == INVALID_SOCKET)
    {
        return false;
    }

    state = State::BETTING;
    currentTurnSocket = firstTurnSocket;

    return true;
}

// 세션을 초기 상태로 리셋
void GameSession::reset()
{
    state = State::WAITING;
    currentTurnSocket = INVALID_SOCKET;
}

// 대기 상태인지 확인
bool GameSession::isWaiting() const noexcept
{
    return state == State::WAITING;
}

// 베팅(게임 진행 중) 상태인지 확인
bool GameSession::isBetting() const noexcept
{
    return state == State::BETTING;
}

// 요청한 소켓이 현재 턴을 가진 유저인지 확인
bool GameSession::isCurrentTurn(SOCKET socket) const noexcept
{
    return state == State::BETTING && currentTurnSocket == socket;
}

// 상대방 소켓으로 턴을 넘김
bool GameSession::passTurnTo(SOCKET opponentSocket)
{
    if (state != State::BETTING || opponentSocket == INVALID_SOCKET)
    {
        return false;
    }

    currentTurnSocket = opponentSocket;
    return true;
}

// 두 장의 카드를 뽑아 반환 (두 플레이어 용)
std::array<std::pair<int, int>, 2> GameSession::dealCards()
{
    return
    {
        deck.DealCard(),
        deck.DealCard()
    };
}