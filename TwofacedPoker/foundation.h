// foundation.h
#pragma once
#include <string>

// 유저 이름과 승리 횟수 표기
class User
{
private:
	std::string ID;
	bool isReady;
	int winCount;
	int chips;

public:
	User(std::string ID, bool isReady = false, int winCount = 0, int chips = 0) : ID(ID), isReady(isReady), winCount(winCount), chips(chips) { }

	std::string getID() const {
		return ID;
	}

	bool getisReady() const {
		return isReady;
	}

	int getwinCount() const {
		return winCount;
	}

	int getchips() const {
		return chips;
	}
};

// 카드의 앞면 뒷면
class Card
{
private:
	int front;
	int back;
public:
	Card(int front, int back) : front(front), back(back) { }

	int getFront() const {
		return front;
	}
	int getBack() const {
		return back;
	}
};