#pragma once

#include "../Common/Foundation.h"

#include <deque>
#include <random>
#include <utility>

class Deck
{
public:
    Deck();

    std::pair<int, int> DealCard();

private:
    void shuffleCard();
    void resupplyCard();

    std::deque<Card> cards;
    std::mt19937 randomEngine;
};