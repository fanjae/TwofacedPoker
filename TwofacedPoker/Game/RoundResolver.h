#pragma once

#include "../Common/foundation.h"

struct PlayerRoundState
{
    int frontCard;
    int backCard;
    BetType betType;
};

class RoundResolver
{
public:
    static RoundResult resolve(const PlayerRoundState& current,const PlayerRoundState& opponent,bool folded);
};