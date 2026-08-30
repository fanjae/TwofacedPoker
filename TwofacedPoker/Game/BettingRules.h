#pragma once

#include "../Common/Foundation.h"

struct BettingState
{
    int chips;
    int frontBet;
    int backBet;
    BetType betType;
};

struct BetDecision
{
    BetResult result;
    int cost;
};

class BettingRules
{
public:
    static BetDecision evaluate(const BettingState& current,const BettingState& opponent,int count,BetType requestedType);
};