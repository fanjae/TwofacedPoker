#pragma once
#include <algorithm>
#include <vector>
#include <random>
#include "foundation.h"
class Deck
{
private:
	std::vector<Card> cards;
public:
	Deck();

	void shuffleCard();
};