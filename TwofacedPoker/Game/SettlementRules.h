#pragma once
#include "../Common/foundation.h"

#include <optional>

struct SettlementState
{
    int currentChips;
    int opponentChips;

    int currentFrontBet;
    int currentBackBet;

    int opponentFrontBet;
    int opponentBackBet;

    int dealerChips;
};

struct SettlementResult
{
    int currentChips;
    int opponentChips;
    int dealerChips;
};

class SettlementRules
{
public:
    static std::optional<SettlementResult> settle(const SettlementState& state, RoundResult roundResult);
};