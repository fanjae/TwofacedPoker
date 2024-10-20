#pragma once
#include <algorithm>
#include <list>
#include <random>
#include "foundation.h"
class Deck
{
private:
	std::list<Card> cards;
public:
	Deck();
	void shuffleCard();
	void resupplyCard();
	int getFrontCard();
	int getBackCard();
};