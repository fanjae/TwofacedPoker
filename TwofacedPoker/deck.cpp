#include <random>
#include <chrono>
#include "deck.h"
#include <iostream>

Deck::Deck()
{
	resupplyCard();
	shuffleCard();
}

void Deck::shuffleCard()
{
	std::cout << "[Game] Now ShuffleCard\n" << std::endl;

	std::vector<Card> vec(cards.begin(), cards.end());

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count(); // 현재 시각 기반 시드 생성
	std::shuffle(vec.begin(), vec.end(), std::default_random_engine(seed));

	cards.assign(vec.begin(), vec.end());
}

void Deck::resupplyCard()
{
	for (int i = 1; i <= 10; i++)
	{
		for (int j = 1; j <= 10; j++)
		{
			if (i == j) continue; // 서로 앞 뒷면 서로 숫자
			cards.push_back(Card(i, j));
		}
	}
	std::cout << "Cards Initalization done.\n";
}

std::pair<int, int> Deck::DealCard()
{
	std::pair<int, int> data;
	data.first = getFrontCard();
	data.second = getBackCard();

	cards.pop_front();
	return data;
}
int Deck::getFrontCard() const
{
	return cards.front().getFront();
}
int Deck::getBackCard() const
{
	return cards.front().getBack();
}

bool Deck::cardEmpty() const
{
	return cards.empty();
}
