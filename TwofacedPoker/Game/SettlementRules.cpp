#include "SettlementRules.h"
#include "../Common/Foundation.h"

#include <algorithm>


// 라운드 결과에 따라 플레이어와 딜러의 칩 정산 처리
std::optional<SettlementResult> SettlementRules::settle(const SettlementState& state, RoundResult roundResult)
{
    // 판정 불가능하면 정산 하지 않음
    if (roundResult == RoundResult::IMPOSSIBLE)
    {
        return std::nullopt;
    }

    // 칩 계산 : 양측이 앞/뒤에 베팅한 모든 칩의 합
    const int pot = state.currentFrontBet + state.currentBackBet + state.opponentFrontBet + state.opponentBackBet;

    // 정산 결과를 담을 구조체
    SettlementResult result
    {
        state.currentChips,
        state.opponentChips,
        state.dealerChips
    };

    // 라운드 결과에 따른 분기
    switch (roundResult)
    {
    case RoundResult::WIN: // 승리
        result.currentChips += pot + state.dealerChips;
        result.dealerChips = 0;
        break;

    case RoundResult::LOSE: // 패배 
        result.opponentChips += pot + state.dealerChips;
        result.dealerChips = 0;
        break;

    case RoundResult::DRAW: // 무승부
        result.dealerChips += pot;
        break;

    case RoundResult::BOTH_WIN: // 양면 베팅 승리
    {
        const int bonusChips = (std::min)(10, result.opponentChips);

        result.opponentChips -= bonusChips;
        result.currentChips += pot + state.dealerChips + bonusChips;
        result.dealerChips = 0;
        break;
    }

    case RoundResult::BOTH_LOSE: // 양면 베팅 패배
    {
        const int penaltyChips = (std::min)(10, result.currentChips);

        result.currentChips -= penaltyChips;
        result.opponentChips += pot + state.dealerChips + penaltyChips;
        result.dealerChips = 0;
        break;
    }

    case RoundResult::IMPOSSIBLE:
    default:
        return std::nullopt;
    }

    return result;
}