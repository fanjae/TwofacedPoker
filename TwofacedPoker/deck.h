#pragma once
#include <algorithm>
#include <list>
#include <random>
#include <algorithm>
#include "foundation.h"
class Deck
{
private:
	std::list<Card> cards;
public:
	Deck();
	void shuffleCard();
	void resupplyCard();
	std::pair<int, int> DealCard();
	int getFrontCard() const;
	int getBackCard() const;
	bool cardEmpty() const;
};