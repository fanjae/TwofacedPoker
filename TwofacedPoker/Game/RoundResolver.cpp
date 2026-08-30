#include "RoundResolver.h"
#include "../Common/Foundation.h"

namespace
{
    // 정상적인 베팅 타입인지 검사 (FRONT, BACK, BOTH)
    bool isPlayableBetType(BetType type)
    {
        return type == BetType::FRONT || type == BetType::BACK || type == BetType::BOTH;
    }
}

// 플레이어 간의 카드와 베팅 타입을 비교하여 승무패 결과를 반환
RoundResult RoundResolver::resolve(const PlayerRoundState& current,const PlayerRoundState& opponent, bool folded)
{
    // 누간군가 기권했을때의 처리
    if (folded)
    {
        if (!isPlayableBetType(opponent.betType))
        {
            return RoundResult::IMPOSSIBLE;
        }

        // 상대가 BOTH 베팅 상대였다면 BOTH_LOSE, 아니면 일반 LOSE 처리
        return opponent.betType == BetType::BOTH ? RoundResult::BOTH_LOSE : RoundResult::LOSE;
    }

    // 일반 비교에는 두 사용자의 베팅 위치가 모두 필요하다.
    if (!isPlayableBetType(current.betType) || !isPlayableBetType(opponent.betType))
    {
        return RoundResult::IMPOSSIBLE;
    }

    // BOTH 대 BOTH는 허용하지 않는다.
    if (current.betType == BetType::BOTH && opponent.betType == BetType::BOTH)
    {
        return RoundResult::IMPOSSIBLE;
    }

    const int currentCards[2]
    {
        current.frontCard,
        current.backCard
    };

    const int opponentCards[2]
    {
        opponent.frontCard,
        opponent.backCard
    };

    // FRONT 0번째 요소(앞면), BACK이면 1번째 요소(뒷면)
    const int currentIndex = current.betType == BetType::FRONT ? 0 : 1;
    const int opponentIndex = opponent.betType == BetType::FRONT ? 0 : 1;

    // 내가 양면 베팅 X
    if (current.betType != BetType::BOTH)
    {
        // 상대가 양면 베팅
        if (opponent.betType == BetType::BOTH)
        {
            const int selectedCard = currentCards[currentIndex];

            // 내가 낸 카드가 상대의 양면 카드보다 모두 작아야만 상대의 승리(나의 BOTH_LOSE)
            if (selectedCard < opponentCards[0] && selectedCard < opponentCards[1])
            {
                return RoundResult::BOTH_LOSE;
            }

            // 그렇지 않으면 나의 승리
            return RoundResult::WIN;
        }

        // 서로 일반 베팅일 경우 카드 숫자 단순 비교
        if (currentCards[currentIndex] > opponentCards[opponentIndex])
        {
            return RoundResult::WIN;
        }

        
        if (currentCards[currentIndex] < opponentCards[opponentIndex])
        {
            return RoundResult::LOSE;
        }

        // 카드 같으면 무승부
        return RoundResult::DRAW;
    }

    // 내가 양면 베팅 했을 때
    if (currentCards[0] > opponentCards[opponentIndex] && currentCards[1] > opponentCards[opponentIndex])
    {
        return RoundResult::BOTH_WIN;
    }

    return RoundResult::LOSE;
}