#include <random>
#include <chrono>
#include "deck.h"

Deck::Deck()
{
	for (int i = 1; i <= 10; i++)
	{
		for (int j = 1; j <= 10; j++)
		{
			if (i == j) continue; // ���� �� �޸� ���� ����
			cards.push_back(Card(i, j));
		}
	}
}

void Deck::shuffleCard()
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count(); // ���� �ð� ��� �õ� ����
	std::shuffle(cards.begin(), cards.end(), std::default_random_engine(seed));
}