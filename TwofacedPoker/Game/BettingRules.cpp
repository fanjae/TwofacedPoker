#include "BettingRules.h"
#include "../Common/Foundation.h"

#include <algorithm>
#include <cstdint>

// 베팅이 규칙에 맞는지 평가하고 CALL/RAISE/IMPOSSIBLE 결과 반환
BetDecision BettingRules::evaluate(const BettingState& current,const BettingState& opponent,int count,BetType requestedType)
{
    // 불가능한 베팅일 경우 반환할 기본 템플릿
    const BetDecision impossible
    {
        BetResult::IMPOSSIBLE,
        0
    };

    // 실제 칩 베팅으로 허용되는 타입
    if (requestedType != BetType::FRONT && requestedType != BetType::BACK && requestedType != BetType::BOTH)
    {
        return impossible;
    }

    if (count <= 0)
    {
        return impossible;
    }

    // 한 라운드 도중 베팅 위치 변경 금지
    if (current.betType != BetType::NONE && current.betType != requestedType)
    {
        return impossible;
    }

    // BOTH 대 BOTH 금지
    if (requestedType == BetType::BOTH && opponent.betType == BetType::BOTH)
    {
        return impossible;
    }

    // 덧셈과 곱셈 오버플로 방지
    const std::int64_t requestedCount = count;
    const std::int64_t currentLevel = (std::max)(current.frontBet, current.backBet);
    const std::int64_t opponentLevel = (std::max)(opponent.frontBet, opponent.backBet);

    // 현재 레벨에 요청된 베팅 개수를 더해 새로운 판돈 레벨 계산
    const std::int64_t newLevel = currentLevel + requestedCount;

    // 양면 베팅은 두 배.
    const std::int64_t cost = requestedType == BetType::BOTH ? requestedCount * 2 : requestedCount;

    // 보유한 칩보다 많이 베팅할 수 없음
    if (cost > current.chips)
    {
        return impossible;
    }

    // 상대의 현재 베팅보다 적게 낼 수 없음
    if (newLevel < opponentLevel)
    {
        return impossible;
    }

    // 상대가 보유한 칩으로 따라올 수 없는 만큼 올리는 행위 금지
    const std::int64_t opponentMaximumLevel = opponentLevel + opponent.chips;

    // 상대가 보유한 칩이상 올리는 RAISE 행위
    if (newLevel > opponentMaximumLevel)
    {
        return impossible;
    }

    // 칩의 개수가 동일하면 CALL
    if (newLevel == opponentLevel)
    {
        return BetDecision
        {
            BetResult::CALL,
            static_cast<int>(cost)
        };
    }

    // 상대방 베팅
    return BetDecision
    {
        BetResult::RAISE,
        static_cast<int>(cost)
    };
}