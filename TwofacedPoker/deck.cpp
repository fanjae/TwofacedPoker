#include <random>
#include <chrono>
#include "deck.h"

Deck::Deck()
{
	for (int i = 1; i <= 10; i++)
	{
		for (int j = 1; j <= 10; j++)
		{
			if (i == j) continue; // 서로 앞 뒷면 서로 숫자
			cards.push_back(Card(i, j));
		}
	}
}

void Deck::shuffleCard()
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count(); // 현재 시각 기반 시드 생성
	std::shuffle(cards.begin(), cards.end(), std::default_random_engine(seed));
}