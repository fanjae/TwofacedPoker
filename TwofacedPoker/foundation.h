// foundation.h
#pragma once
#include <string>

// 유저 이름과 승리 횟수 표기
class User
{
private:
	std::string name;
	int winCount;
public:
	User(std::string name, int winCount) : name(name), winCount(winCount) { }

	std::string getName() const {
		return name;
	}
	int getwinCount() const {
		return winCount;
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