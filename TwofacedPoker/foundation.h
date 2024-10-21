// foundation.h
#pragma once
#include <string>
enum class InitType
{
	INIT,
	PROGRESS
};

enum class GameType
{
	WIN,
	LOSE,
	DRAW,
	INIT
};

enum class BetType
{
	FRONT,
	BOTH,
	BACK,
	NONE
};

class DualValue
{
protected:
	int front;
	int back;

public:
	DualValue(int front, int back) : front(front), back(back) {}

	int getFront() const {
		return front;
	}

	int getBack() const {
		return back;
	}

	void setFront(int value) {
		front = value;
	}

	void setBack(int value) {
		back = value;
	}
};

// 카드 관련
class Card : public DualValue
{
public:
	Card(int front, int back) : DualValue(front, back) { }
};

// 베팅된 칩 관련
class Bet : public DualValue
{
public:
	Bet(int front, int back) : DualValue(front, back) { }

};

// 유저 이름과 승리 횟수 표기
class User
{
private:
	int userNumber; 
	std::string ID;
	bool isReady;
	int winCount;
	int chips;
	Bet bet_chips;
	Card now_cards;
	BetType bet_type;
	
public:
	User(int userNumber, std::string ID, bool isReady = false, int winCount = 0, int chips = 0) : userNumber(userNumber), ID(ID), isReady(isReady), winCount(winCount), chips(chips), bet_chips(0, 0), now_cards(0, 0), bet_type(BetType::NONE) { }

	int getuserNumber() const {
		return userNumber;
	}

	std::string getID() const {
		return ID;
	}

	bool getisReady() const {
		return isReady;
	}

	int getwinCount() const {
		return winCount;
	}

	int getChips() const {
		return chips;
	}

	void setChips(int value) {
		this->chips = value;
	}
	void setisReady(bool value) {
		this->isReady = value;
	}

	int getFrontBet() const
	{
		return bet_chips.getFront();
	}
	int getBackBet() const
	{
		return bet_chips.getBack();
	}
	void setFrontBet(int value) {
		bet_chips.setFront(value);
	}
	void setBackBet(int value) {
		bet_chips.setBack(value);
	}
	int getFrontCard() const
	{
		return now_cards.getFront();
	}
	int getBackCard() const
	{
		return now_cards.getBack();
	}
	void setFrontCard(int value) {
		now_cards.setFront(value);
	}
	void setBackCard(int value) {
		now_cards.setBack(value);
	}
};