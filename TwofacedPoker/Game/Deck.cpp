#include "../Common/Foundation.h"
#include "deck.h"

#include <algorithm>
#include <iostream>
#include <random>

// 덱 생성(난수 엔진을 통해 초기화)
Deck::Deck() : randomEngine(std::random_device{}())
{
    resupplyCard();
}

// 데겡 있는 카드를 무작위로 섞음
void Deck::shuffleCard()
{
    std::cout << "[Game] Now ShuffleCard\n";

    std::shuffle(cards.begin(),cards.end(),randomEngine);
}

// 덱 초기화 및 카드 보충
void Deck::resupplyCard()
{
    cards.clear();

    // 앞뒷면 숫자가 1~10인 카드 생성
    for (int front = 1; front <= 10; ++front)
    {
        for (int back = 1; back <= 10; ++back)
        {
            // 앞뒷면 숫자가 동일한 카드 제외
            if (front == back)
            {
                continue;
            }

            cards.emplace_back(front, back);
        }
    }
    // 카드 생성 후 섞기
    shuffleCard();

    std::cout << "[Game] Cards initialization done.\n";
}

// 덱에서 카드 한 장을 뽑아 반환
std::pair<int, int> Deck::DealCard()
{
    // 카드가 다 떨어졌다면 다시 채움
    if (cards.empty())
    {
        resupplyCard();
    }

    // 맨 위 카드를 뽑고 덱에서 제거
    const Card card = cards.front();
    cards.pop_front();

    std::cout << "[Card] Front: " << card.getFront() << ", Back: " << card.getBack() << '\n';

    return
    {
        card.getFront(),
        card.getBack()
    };
}